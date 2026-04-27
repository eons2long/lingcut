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

#ifndef QWEN3ASRJOB_H
#define QWEN3ASRJOB_H

#include "abstractjob.h"

class Qwen3AsrJob : public AbstractJob
{
    Q_OBJECT
public:
    Qwen3AsrJob(const QString &name,
                const QString &iWavFile,
                const QString &oSrtFile,
                int maxLength,
                QThread::Priority priority = Settings.jobPriority());
    virtual ~Qwen3AsrJob();

public slots:
    void start() override;

protected slots:
    void onReadyRead() override;
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus) override;

private:
    bool writeSrtFromOutput();
    const QString m_iWavFile;
    const QString m_oSrtFile;
    const int m_maxLength;
    int m_previousPercent;
    QString m_output;
    QString m_outputTextFile;
    QString m_outputLogFile;
};

#endif // QWEN3ASRJOB_H
