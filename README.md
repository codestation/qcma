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

## Headless version

There is a qcma_cli binary that doesn't need a X session running (useful for servers).
The daemon provides minimal interaction via unix signals. Here are some usage examples:

#### Send a signal to refresh the database

```
kill -HUP $(pgrep qcma_cli)
```

#### Send a signal to stop the daemon

```
kill $(pgrep qcma_cli)
```

## Downloads (Linux)

Packages for Ubuntu, Fedora, Debian and openSUSE are available [here](https://software.opensuse.org/download/package?project=home:codestation&package=qcma).

## Downloads (Windows and macOS)

Installers for Windows and macOS are available [here](https://github.com/codestation/qcma/releases/latest).

## Dependencies
* [Qt 5.x](http://www.qt.io/)

* [VitaMTP fork](https://github.com/codestation/vitamtp)

* [FFmpeg](http://www.ffmpeg.org/)

* [libnotify](http://library.gnome.org/devel/notification-spec/)

#### Where do I get the source code?
Check the GitHub repo here: https://github.com/codestation/qcma

#### I want to contribute 
Contact me on [GitHub](https://github.com/codestation/) 

## Thanks to
[Yifan Lu](https://github.com/yifanlu/vitamtp/) - for the vitamtp library and
the reference implementation of OpenCMA.

Other contributors: check [here](https://github.com/codestation/qcma/graphs/contributors)

#### License
GPLv3
