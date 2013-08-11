QCMA
====

QCMA is an cross-platform application to provide a Open Source implementation
of the original Content Manager Assistant that comes with the PS Vita. QCMA is
meant to be compatible with Linux, Windows and MAC OS X.

## Features

The aim of this project is to provide a implementation that is on par with the
official CMA and also offer some features missing in the original one.

#### Implemented features missing in OpenCMA (Yifan Lu CLI application)
* Metadata for PSP savedatas.
* Basic metadata for single songs (album, artist, title).
* Basic metadata for videos (duration, dimensions).
* Basic metadata for photos (dimensions).
* Easy wireless pairing (show PIN to the user when a Vita is detected).
* Ability to restart the connection if the Vita is reconnected (on test).
* Ability to change the connection mode (usb/wireless) on the fly (on test).

#### TODO:
* Fix remaining bugs with thread synchronizations.
* Implement thumbnails for videos and album art for music.
* Folder categories for music/videos.
* SQLite backend for database.
* Fix wireless streaming for music/videos.

## Planned features
* **Backup browser**: provide a human readable listing of the games saved on the
computer (name, icon, side on disk) and provide some actions (delete savedata,
patches or the whole game without need of the Vita). Also some sort of interface
to prepare VHBL homebrew in the folder of the exploited savedata.

* **DLNA bridge**: connect an existing DLNA server to interface with the Vita
using the wireless streaming feature.

#### Dependencies
* [Qt 4.x or 5.x](http://qt-project.org/)

* [VitaMTP](https://github.com/yifanlu/VitaMTP)

* [MediaInfo](http://mediaarea.net/en/MediaInfo)


#### Where do I get the source code?
Check the GitHub repo here: https://github.com/codestation/qcma

#### I want to contribute 
Contact me on [GitHub](https://github.com/codestation/) 

#### Thanks to
[Yifan Lu](https://github.com/yifanlu/vitamtp/) - for the vitamtp library and
the reference implementation of OpenCMA.

#### License
GPL v3: since some parts of QCMA are based on the reference implementation of
OpenCMA.
