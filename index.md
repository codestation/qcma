**Note: The downloads at the top are the source code. Go near the bottom of this page for Windows/OSX installers**

Qcma
====

Qcma is a cross-platform application to provide a Open Source implementation
of the original Content Manager Assistant that comes with the PS Vita. Qcma is
meant to be compatible with Linux, Windows and MAC OS X.

## Features

The aim of this project is to provide an implementation that is on par with the
official CMA and also offer some features missing in the original one.

#### Implemented features.
* Metadata for PSP savedatas.
* Basic metadata for single songs (album, artist, title, cover art).
* Basic metadata for videos (duration, dimensions, thumbnail).
* Basic metadata for photos (dimensions, thumbnails).
* Simple backup browser: view and delete the backups on your PC without a Vita.
* Easy wireless pairing (show PIN to the user when a Vita is detected).
* Ability to restart the connection if the Vita is reconnected.

## Headless version

There is a qcma_cli binary that doesn't need a X session running (useful for servers).
The daemon provides minimal interaction via UNIX signals.

* **SIGHUP**: Refreshes the database.
* **SIGTERM** or **SIGINT**: Shuts down the process but waits until the current event is finished.

## Dependencies
* [Qt 5.x](http://qt-project.org/)

* [VitaMTP fork](https://github.com/codestation/vitamtp)

* [FFmpeg](hhttp://www.ffmpeg.org/)

## Downloads (Latest version: **0.4.1**)

[Linux repos (0.4.1)](https://software.opensuse.org/download/package.iframe?project=home:codestation&package=qcma)

[Windows Installer (0.4.1)](https://github.com/codestation/qcma/releases/download/v0.4.1/Qcma_setup-0.4.1.exe)

[macOS dmg (0.4.1-1)](https://github.com/codestation/qcma/releases/download/v0.4.1/Qcma_0.4.1-1.dmg)

#### Where do I get the source code?
Check the GitHub repo here: https://github.com/codestation/qcma

#### I want to contribute 
Contact me on [GitHub](https://github.com/codestation/) 

## Thanks to
[Yifan Lu](https://github.com/yifanlu/vitamtp/) - for the vitamtp library and
the reference implementation of OpenCMA.

[Xian Nox](https://github.com/xiannox) - for the Wiki and various contributions.

#### License
GPL v3: since some parts of QCMA are based on the reference implementation of
OpenCMA.
