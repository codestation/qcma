QCMA
====

QCMA is a cross-platform application to provide a Open Source implementation
of the original Content Manager Assistant that comes with the PS Vita. QCMA is
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

#### TODO:
* Complete categories for music.
* SQLite backend for database.

#### Planned features
* **Android version**: port of Qcma to Android.

## Dependencies
* [Qt 4.x or 5.x](http://qt-project.org/)

* [VitaMTP fork](https://github.com/codestation/VitaMTP)

* [FFmpeg](hhttp://www.ffmpeg.org/)


#### Where do I get the source code?
Check the GitHub repo here: https://github.com/codestation/qcma

#### I want to contribute 
Contact me on [GitHub](https://github.com/codestation/) 

## Thanks to
[Yifan Lu](https://github.com/yifanlu/vitamtp/) - for the vitamtp library and
the reference implementation of OpenCMA.

[Xian Nox] (https://github.com/xiannox) - for the Wiki and various contributions.

#### License
GPL v3: since some parts of QCMA are based on the reference implementation of
OpenCMA.
