# macOS arm64

The native macOS build targets Apple Silicon and macOS 13 or newer. It uses
current Homebrew packages and Qt 6, builds VitaMTP before Qcma, produces an
application bundle and wraps it in a DMG.

Run on an Apple Silicon host:

```sh
./buildscripts/macos/prepare.sh
./buildscripts/macos/build.sh
```

Homebrew itself must already be installed. `prepare.sh` installs or updates the
dependencies declared in `Brewfile` and is safe to run again. `build.sh`
expects VitaMTP in a sibling `vitamtp` checkout and writes the DMG to
`artifacts/macos`. Both can be overridden:

```sh
./buildscripts/macos/build.sh \
  --vitamtp-source /path/to/vitamtp \
  --output-dir /path/to/artifacts \
```

`macdeployqt` deploys Qt frameworks and plugins. `dylibbundler` adds the
Homebrew/VitaMTP dependency closure, after which the script verifies arm64
architecture, macOS 13 deployment targets and the absence of build-host paths.

The app receives an ad-hoc signature because Apple Silicon requires signed
Mach-O code, but it is not signed with Developer ID or notarized. Gatekeeper
will therefore warn when opening a downloaded DMG. Launch, DMG installation,
USB and Wi-Fi tests must be completed on a real Apple Silicon host before
publishing it.
