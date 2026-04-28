# LingCut - a cross-platform **video editor** focused on subtitles

<div align="center">

<img src="https://www.shotcut.org/assets/img/screenshots/Shotcut-18.11.18.png" alt="screenshot" />

</div>

LingCut is a Shotcut-based video editor focused on creator workflows, including
local ASR subtitle generation and timeline editing improvements.

This repository is a fork of [Shotcut](https://github.com/mltframework/shotcut),
a free, open source, cross-platform video editor.

- Upstream project: https://github.com/mltframework/shotcut
- Upstream features: https://www.shotcut.org/features/

## Install

LingCut release builds are not published yet. For the original Shotcut releases,
see https://www.shotcut.org/download/.

## Contributors

- Dan Dennedy <<http://www.dennedy.org>> : main author
- Brian Matherly <<code@brianmatherly.com>> : contributor

## Dependencies

LingCut's direct (linked or hard runtime) dependencies are:

- [MLT](https://www.mltframework.org/): multimedia authoring framework
- [Qt 6 (6.4 minimum)](https://www.qt.io/): application and UI framework
- [FFTW](https://fftw.org/)
- [FFmpeg](https://www.ffmpeg.org/): multimedia format and codec libraries
- [Frei0r](https://www.dyne.org/software/frei0r/): video plugins
- [SDL](http://www.libsdl.org/): cross-platform audio playback

See https://shotcut.org/credits/ for a more complete list including indirect
and bundled dependencies.

## License

GPLv3. See [COPYING](COPYING).

## How to build

**Warning**: building LingCut should only be reserved to beta testers or contributors who know what they are doing.

### Qt Creator

The fastest way to build and try the LingCut development version is through [Qt Creator](https://www.qt.io/download#qt-creator).

### From command line

First, check dependencies are satisfied and various paths are correctly set to find different libraries and include files (Qt, MLT, frei0r and so forth).

#### Configure

In a new directory in which to make the build (separate from the source):

```
cmake -DCMAKE_INSTALL_PREFIX=/usr/local/ /path/to/lingcut
```

We recommend using the Ninja generator by adding `-GNinja` to the above command line.

#### Build

```
cmake --build .
```

#### Install

If you do not install, LingCut may fail when you run it because it cannot locate its QML
files that it reads at run-time.

```
cmake --install .
```

## Translation

LingCut currently inherits upstream Shotcut translations. Translation-only
updates should wait until a LingCut translation workflow is set up.
