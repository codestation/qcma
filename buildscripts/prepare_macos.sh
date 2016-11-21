#!/bin/bash

set -eu

show_usage() {
    echo -e "Usage: $0 <host>"
}

if [ $# -lt 1 ]
then
    show_usage
    exit 1
fi

SERVER_HOST=$1

QT_VERSION=5.7
QT_REVISION=${QT_VERSION}.0

QT_PKG=qt-opensource-mac-x64-clang-5.7.0.dmg
NODE_PKG=node-v7.1.0.pkg
XCODE_PKG=Xcode_8.xip

scp ${NODE_PKG} ${SERVER_HOST}:
scp ${QT_PKG} ${SERVER_HOST}:
scp ${XCODE_PKG} ${SERVER_HOST}:
scp qt-noninteractive.qs ${SERVER_HOST}:

read -n1 -r -p "Unpack Xcode, then press space to continue..." key

ssh -T "${SERVER_HOST}" << EOSSH
#!/bin/bash

set -eu

# Install xcode

if [ ! -d /Applications/Xcode.app ]; then
  mv Xcode.app /Applications/
  xcodebuild -license accept
fi

# Install nodejs
if [ ! -f /usr/local/bin/npm ]; then
  sudo installer -pkg ${NODE_PKG} -target /usr/local
fi

# Install appdmg
if [ ! -f /usr/local/bin/appdmg ]; then
  sudo npm install -g appdmg
fi

# Install Qt
if [ ! -d "~/Qt${QT_REVISION}/${QT_VERSION}" ]; then
  hdiutil attach -noverify ${QT_PKG}
  VOLUME_NAME="${QT_PKG%.*}"
  /Volumes/${VOLUME_NAME}/${VOLUME_NAME}.app/Contents/MacOS/${VOLUME_NAME} --script qt-noninteractive.qs
  hdiutil detach /Volumes/${VOLUME_NAME}
fi

# Fix macOS compilation
PATCH_FILE=~/Qt5.7.0/5.7/clang_64/mkspecs/features/mac/default_pre.prf
sed -i '' 's#isEmpty($$list($$system("/usr/bin/xcrun -find xcrun 2>/dev/null")))#isEmpty($$list($$system("/usr/bin/xcrun -find xcodebuild 2>/dev/null")))#' \${PATCH_FILE}

# Install brew
if [ ! -f /usr/local/bin/brew ]; then
  /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi

# Install build deps
brew install libtool automake gettext libusb pkg-config libxml2 ffmpeg
brew link gettext --force

EOSSH
