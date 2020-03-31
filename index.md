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

#### TODO:
* Complete categories for music.
* Persistent backend for database.

#### Planned features
* **Android version**: port of Qcma to Android.

## Headless version

There is a qcma_cli binary that doesn't need a X session running (useful for servers).
The daemon provides minimal interaction via UNIX signals.

* **SIGHUP**: Refreshes the database.
* **SIGTERM** or **SIGINT**: Shuts down the process but waits until the current event is finished.

## Dependencies
* [Qt 5.x](http://qt-project.org/)

* [VitaMTP fork](https://github.com/codestation/vitamtp)

* [FFmpeg](hhttp://www.ffmpeg.org/)

## Downloads (Latest version: **0.4.0**)

### Linux

**Ubuntu PPA**

``` sh
sudo add-apt-repository ppa:codestation404/qcma
sudo apt-get update
sudo apt-get install qcma
```

[Archlinux AUR](https://aur.archlinux.org/packages/qcma)

[Ubuntu 14.04 (64 bits)](http://codestation.nekmo.com/qcma/0.3.12/ubuntu_trusty)

[Ubuntu 16.04 (64 bits)](http://codestation.nekmo.com/qcma/0.3.12/ubuntu_xenial)

[Debian Jessie (64 bits)](http://codestation.nekmo.com/qcma/0.3.12/debian_jessie/)

[Fedora 24 (64 bits)](http://codestation.nekmo.com/qcma/0.3.12/fedora_24/)

[openSUSE Leap 42.1 (64 bits)](http://codestation.nekmo.com/qcma/0.3.12/opensuse_42.1/)

### Windows

[Windows Installer (0.3.13-2)](http://codestation.nekmo.com/qcma/0.3.13/windows/Qcma_setup_0.3.13-2.exe)

### OS X

[OS X dmg (0.3.12-2)](http://codestation.nekmo.com/qcma/0.3.12/osx/qcma-0.3.12-2.dmg)

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