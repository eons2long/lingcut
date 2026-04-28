# LingCut - 跨平台**视频编辑器**

[English](README.md)

<div align="center">

<img src="https://www.shotcut.org/assets/img/screenshots/Shotcut-18.11.18.png" alt="screenshot" />

</div>

LingCut 是基于 Shotcut 的视频编辑器，面向创作者工作流做了一些改进，
包括本地 ASR 字幕生成和时间线编辑体验优化。

本仓库 fork 自 [Shotcut](https://github.com/mltframework/shotcut)。
Shotcut 是一个免费、开源、跨平台的视频编辑器。

- LingCut 仓库：https://github.com/eons2long/lingcut
- 上游项目：https://github.com/mltframework/shotcut
- 上游功能介绍：https://www.shotcut.org/features/

## 安装

LingCut 暂未发布正式安装包。原版 Shotcut 的发布版本可以在这里下载：

https://www.shotcut.org/download/

## 贡献者

- Dan Dennedy <<http://www.dennedy.org>>：主要作者
- Brian Matherly <<code@brianmatherly.com>>：贡献者

## 依赖

LingCut 的直接依赖包括：

- [MLT](https://www.mltframework.org/)：多媒体创作框架
- [Qt 6（最低 6.4）](https://www.qt.io/)：应用和 UI 框架
- [FFTW](https://fftw.org/)
- [FFmpeg](https://www.ffmpeg.org/)：多媒体格式和编解码库
- [Frei0r](https://www.dyne.org/software/frei0r/)：视频插件
- [SDL](http://www.libsdl.org/)：跨平台音频播放

更多直接和间接依赖见 Shotcut 上游 credits：

https://shotcut.org/credits/

## 许可证

GPLv3。见 [COPYING](COPYING)。

## 如何构建

**注意**：构建 LingCut 适合测试者或熟悉项目构建流程的贡献者。

### Qt Creator

最快的开发构建方式是使用 [Qt Creator](https://www.qt.io/download#qt-creator)。

### 命令行

首先确认依赖已经安装，并且 Qt、MLT、frei0r 等库和头文件路径配置正确。

#### 配置

在源码目录外新建一个构建目录，然后运行：

```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr/local/ /path/to/lingcut
```

建议使用 Ninja 生成器，可以追加 `-GNinja`。

#### 构建

```bash
cmake --build .
```

#### 安装

如果不执行安装步骤，LingCut 运行时可能找不到 QML 文件。

```bash
cmake --install .
```

## 翻译

LingCut 目前继承 Shotcut 上游翻译。翻译类 PR 建议等 LingCut 自己的翻译流程确定后再提交。
