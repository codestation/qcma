#!/bin/bash

set -eu

show_usage() {
    echo -e "Usage: $0 <host> <branch> [config]"
}

if [ $# -lt 1 ]
then
    show_usage
    exit 1
fi

SERVER_HOST=$1
BRANCH=$2

QT_VERSION=5.7
QT_REVISION=${QT_VERSION}.0
QCMA_SOURCES=~/projects/qcma

VERSION=$(git -C "${QCMA_SOURCES}" describe --tags --abbrev=8)
VERSION=${VERSION#v*}

if [ $# -ge 3 ]; then
    CONFIG=$3
else
    CONFIG=
fi

case "$CONFIG" in
  *DISABLE_FFMPEG*)
    BUILD_MODE=_noffmpeg
    ;;
  *)
    BUILD_MODE=
    ;;
esac

git -C "${QCMA_SOURCES}" bundle create qcma.bundle --all
scp "${QCMA_SOURCES}/qcma.bundle" $SERVER_HOST:qcma.bundle

ssh -T "${SERVER_HOST}" << EOSSH
#!/bin/bash

set -eu
QCMA_DIR="\$HOME/qcma"
rm -rf "\${QCMA_DIR}"
git clone -b ${BRANCH} qcma.bundle "\${QCMA_DIR}"

rm -rf qcma_build
mkdir qcma_build

pushd qcma_build
PATH=~/Qt${QT_REVISION}/${QT_VERSION}/clang_64/bin:/usr/local/bin:\$PATH
lrelease "\${QCMA_DIR}/qcma.pro"
qmake "\${QCMA_DIR}/qcma.pro" $CONFIG
make -j2
popd

rm -rf \${HOME}/qcma_output
mkdir \${HOME}/qcma_output
mv qcma_build/gui/qcma.app \${HOME}/qcma_output/Qcma.app
cp \${QCMA_DIR}/buildscripts/macos/* \${HOME}/qcma_output/

pushd \${HOME}/qcma_output/
macdeployqt Qcma.app -appstore-compliant
appdmg appdmg.json Qcma_${VERSION}${BUILD_MODE}.dmg
echo "Created Qcma_${VERSION}${BUILD_MODE}.dmg"
popd
EOSSH

scp ${SERVER_HOST}:qcma_output/Qcma_${VERSION}${BUILD_MODE}.dmg .
