/*
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qwen3asrjob.h"

#include "Logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QThread>

namespace {

struct QwenSegment
{
    int startMs;
    int endMs;
    QString text;
};

struct Cue
{
    int startMs;
    int endMs;
    QString text;
};

static QString srtTime(int ms)
{
    ms = qMax(0, ms);
    const int hours = ms / 3600000;
    ms %= 3600000;
    const int minutes = ms / 60000;
    ms %= 60000;
    const int seconds = ms / 1000;
    const int milliseconds = ms % 1000;
    return QStringLiteral("%1:%2:%3,%4")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(milliseconds, 3, 10, QLatin1Char('0'));
}

static bool isSplitPunctuation(QChar c)
{
    static const QString marks = QStringLiteral("，,。.!！？?；;、：:…");
    return marks.contains(c);
}

static bool isDiscardedPunctuation(QChar c)
{
    static const QString marks = QStringLiteral("，,。.!！？?；;、：:（）()《》<>“”\"‘’'【】[]—-…");
    return marks.contains(c);
}

static QStringList splitText(const QString &text, int maxLength)
{
    QStringList parts;
    QString current;
    maxLength = qBound(10, maxLength, 100);

    auto flush = [&]() {
        const QString trimmed = current.simplified();
        if (!trimmed.isEmpty()) {
            parts << trimmed;
        }
        current.clear();
    };

    for (QChar c : text) {
        const bool splitPunctuation = isSplitPunctuation(c);
        const bool discardedPunctuation = isDiscardedPunctuation(c);
        if (!discardedPunctuation) {
            current.append(c);
        }

        const int length = current.simplified().size();
        if (splitPunctuation) {
            flush();
        } else if (length >= maxLength) {
            flush();
        }
    }
    flush();
    return parts;
}

static QList<Cue> makeCues(const QList<QwenSegment> &segments, int maxLength)
{
    QList<Cue> cues;
    for (const QwenSegment &segment : segments) {
        const QStringList parts = splitText(segment.text, maxLength);
        if (parts.isEmpty()) {
            continue;
        }

        int totalWeight = 0;
        for (const QString &part : parts) {
            totalWeight += qMax(1, part.size());
        }

        int startMs = segment.startMs;
        for (int i = 0; i < parts.size(); ++i) {
            int endMs = segment.endMs;
            if (i + 1 < parts.size()) {
                const double ratio = double(qMax(1, parts[i].size())) / double(qMax(1, totalWeight));
                endMs = qMin(segment.endMs, startMs + qMax(500, qRound((segment.endMs - startMs) * ratio)));
                totalWeight -= qMax(1, parts[i].size());
            }
            if (endMs <= startMs) {
                endMs = startMs + 500;
            }
            cues.append({startMs, endMs, parts[i]});
            startMs = endMs;
        }
    }
    return cues;
}

static bool parseSegments(const QString &output, QList<QwenSegment> &segments, QString &errorString)
{
    static const QRegularExpression segmentRegex(
        QStringLiteral("^\\s*(\\d+(?:\\.\\d+)?)\\s+--\\s+(\\d+(?:\\.\\d+)?)\\s*:\\s*(.+?)\\s*$"));
    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QRegularExpressionMatch match = segmentRegex.match(line);
        if (!match.hasMatch()) {
            continue;
        }

        const int startMs = qRound(match.captured(1).toDouble() * 1000.0);
        const int endMs = qRound(match.captured(2).toDouble() * 1000.0);
        const QString text = match.captured(3).trimmed();
        if (endMs > startMs && !text.isEmpty()) {
            segments.append({startMs, endMs, text});
        }
    }

    if (segments.isEmpty()) {
        errorString = QObject::tr("No timed recognition result found in Qwen3-ASR output.");
        return false;
    }
    return true;
}

} // namespace

Qwen3AsrJob::Qwen3AsrJob(const QString &name,
                         const QString &iWavFile,
                         const QString &oSrtFile,
                         int maxLength,
                         QThread::Priority priority)
    : AbstractJob(name, priority)
    , m_iWavFile(iWavFile)
    , m_oSrtFile(oSrtFile)
    , m_maxLength(maxLength)
    , m_previousPercent(0)
{
    disconnect(this, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(this, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    setTarget(oSrtFile);
}

Qwen3AsrJob::~Qwen3AsrJob()
{
    LOG_DEBUG() << "begin";
}

void Qwen3AsrJob::start()
{
    const QString exePath = Settings.qwen3AsrExe();
    const QDir modelDir(Settings.qwen3AsrModelDir());

    m_output.clear();
    m_outputTextFile = m_oSrtFile + QStringLiteral(".qwen3.txt");
    m_outputLogFile = m_oSrtFile + QStringLiteral(".qwen3.log");
    QFile::remove(m_outputTextFile);
    QFile::remove(m_outputLogFile);
    setReadChannel(QProcess::StandardOutput);
    setProcessChannelMode(QProcess::SeparateChannels);
    setStandardOutputFile(m_outputTextFile, QIODevice::Truncate);
    setStandardErrorFile(m_outputLogFile, QIODevice::Truncate);

    QStringList args;
    args << QStringLiteral("--print-args=false");
    args << QStringLiteral("--silero-vad-model=%1").arg(Settings.qwen3AsrVadModel());
    args << QStringLiteral("--qwen3-asr-conv-frontend=%1")
                .arg(modelDir.absoluteFilePath(QStringLiteral("conv_frontend.onnx")));
    args << QStringLiteral("--qwen3-asr-encoder=%1")
                .arg(modelDir.absoluteFilePath(QStringLiteral("encoder.int8.onnx")));
    args << QStringLiteral("--qwen3-asr-decoder=%1")
                .arg(modelDir.absoluteFilePath(QStringLiteral("decoder.int8.onnx")));
    args << QStringLiteral("--qwen3-asr-tokenizer=%1")
                .arg(modelDir.absoluteFilePath(QStringLiteral("tokenizer")));
    args << QStringLiteral("--qwen3-asr-max-new-tokens=512");
    args << QStringLiteral("--provider=cpu");

#if QT_POINTER_SIZE == 4
    auto threadCount = 1;
#else
    auto threadCount = qMax(1, QThread::idealThreadCount() - 1);
#endif
    args << QStringLiteral("--num-threads=%1").arg(threadCount);
    args << m_iWavFile;

    LOG_DEBUG() << exePath + " " + args.join(' ');
    AbstractJob::start(exePath, args);
    emit progressUpdated(m_item, 0);
}

void Qwen3AsrJob::onReadyRead()
{
    const QString output = QString::fromUtf8(readAllStandardOutput());
    if (!output.isEmpty()) {
        m_output.append(output);
    }

    const QString msg = QString::fromUtf8(readAllStandardError());
    if (!msg.isEmpty()) {
        appendToLog(msg);
        if (msg.contains(QStringLiteral("Started")) && m_previousPercent < 5) {
            emit progressUpdated(m_item, 5);
            m_previousPercent = 5;
        }
        QCoreApplication::processEvents();
    }
}

void Qwen3AsrJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const QString remainingOutput = QString::fromUtf8(readAllStandardOutput());
    if (!remainingOutput.isEmpty()) {
        m_output.append(remainingOutput);
    }

    const QString remainingLog = QString::fromUtf8(readAllStandardError());
    if (!remainingLog.isEmpty()) {
        appendToLog(remainingLog);
    }

    QFile outputFile(m_outputTextFile);
    if (outputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QByteArray outputBytes = outputFile.readAll();
        m_output.append(QString::fromUtf8(outputBytes));
        appendToLog(tr("Qwen3-ASR stdout: %1 (%2 bytes)\n").arg(m_outputTextFile).arg(outputBytes.size()));
    } else if (!m_outputTextFile.isEmpty()) {
        const QString message = tr("Failed to read Qwen3-ASR stdout: %1").arg(m_outputTextFile);
        LOG_ERROR() << message;
        appendToLog(message + QLatin1Char('\n'));
    }

    QFile logFile(m_outputLogFile);
    if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        appendToLog(QString::fromUtf8(logFile.readAll()));
    }

    if (exitStatus == QProcess::NormalExit && exitCode == 0 && !stopped()) {
        if (!writeSrtFromOutput()) {
            exitCode = 1;
        }
    }
    AbstractJob::onFinished(exitCode, exitStatus);
}

bool Qwen3AsrJob::writeSrtFromOutput()
{
    QString output = m_output;
    const QString jobLog = log();
    if (!jobLog.isEmpty()) {
        output.append(QLatin1Char('\n'));
        output.append(jobLog);
    }
    LOG_DEBUG() << "Qwen3-ASR output chars" << output.size() << "buffer" << m_output.size()
                << "job log" << jobLog.size() << "text file" << m_outputTextFile;

    QList<QwenSegment> segments;
    QString errorString;
    if (!parseSegments(output, segments, errorString)) {
        LOG_ERROR() << errorString;
        appendToLog(errorString + QLatin1Char('\n'));
        return false;
    }

    const QList<Cue> cues = makeCues(segments, m_maxLength);
    if (cues.isEmpty()) {
        appendToLog(tr("No subtitles generated from Qwen3-ASR output.\n"));
        return false;
    }

    QFile srtFile(m_oSrtFile);
    if (!srtFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        const QString errorString = tr("Failed to write SRT file: %1").arg(m_oSrtFile);
        LOG_ERROR() << errorString;
        appendToLog(errorString + QLatin1Char('\n'));
        return false;
    }

    QTextStream stream(&srtFile);
    stream.setEncoding(QStringConverter::Utf8);
    for (int i = 0; i < cues.size(); ++i) {
        stream << i + 1 << "\n";
        stream << srtTime(cues[i].startMs) << " --> " << srtTime(cues[i].endMs) << "\n";
        stream << cues[i].text << "\n\n";
    }
    return true;
}
