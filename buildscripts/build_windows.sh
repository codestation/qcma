#!/bin/bash

set -ex

SERVER_HOST=$1

VERSION=$(git describe --tags --abbrev=8)

scp windows/driver/*.exe ${SERVER_HOST}:
scp windows/qcma.nsi ${SERVER_HOST}:qcma.nsi
scp COPYING ${SERVER_HOST}:COPYING.rtf
scp gui/resources/images/qcma.ico ${SERVER_HOST}:qcma.ico

ssh -T "${SERVER_HOST}" << EOSSH
#!/bin/bash
set -ex

for arch in i686 x86_64; do
  if [ "\$arch" == "i686" ]; then
    bits=32
    seh=libgcc_s_dw2-1.dll
  else
    bits=64
    seh=libgcc_s_seh-1.dll
  fi
  rm -rf win_\${arch}
  mkdir win_\${arch}
  OUT=\$(pwd)

  pushd /cygdrive/c/ffmpeg-3.1.4-win\${bits}-shared/bin
  cp avcodec-57.dll "\${OUT}/win_\${arch}/"
  cp avformat-57.dll "\${OUT}/win_\${arch}/"
  cp avutil-55.dll "\${OUT}/win_\${arch}/"
  cp swscale-4.dll "\${OUT}/win_\${arch}/"
  cp swresample-2.dll "\${OUT}/win_\${arch}/"

  pushd /cygdrive/c/msys64/mingw\${bits}/bin
  cp libfreetype-6.dll "\${OUT}/win_\${arch}/"
  cp \${seh} "\${OUT}/win_\${arch}/"
  cp libglib-2.0-0.dll "\${OUT}/win_\${arch}/"
  cp libgraphite2.dll "\${OUT}/win_\${arch}/"
  cp libharfbuzz-0.dll "\${OUT}/win_\${arch}/"
  cp libiconv-2.dll "\${OUT}/win_\${arch}/"
  cp libicudt57.dll "\${OUT}/win_\${arch}/"
  cp libicuin57.dll "\${OUT}/win_\${arch}/"
  cp libicuuc57.dll "\${OUT}/win_\${arch}/"
  cp libjpeg-8.dll "\${OUT}/win_\${arch}/"
  cp libpcre-1.dll "\${OUT}/win_\${arch}/"
  cp libpcre16-0.dll "\${OUT}/win_\${arch}/"
  cp libpng16-16.dll "\${OUT}/win_\${arch}/"
  cp libsqlite3-0.dll "\${OUT}/win_\${arch}/"
  cp libstdc++-6.dll "\${OUT}/win_\${arch}/"
  cp libtiff-5.dll "\${OUT}/win_\${arch}/"
  cp libusb-1.0.dll "\${OUT}/win_\${arch}/"
  cp libvitamtp-5.dll "\${OUT}/win_\${arch}/"
  cp libwinpthread-1.dll "\${OUT}/win_\${arch}/"
  cp libxml2-2.dll "\${OUT}/win_\${arch}/"
  cp Qt5Core.dll "\${OUT}/win_\${arch}/"
  cp Qt5Gui.dll "\${OUT}/win_\${arch}/"
  cp Qt5Sql.dll "\${OUT}/win_\${arch}/"
  cp Qt5Network.dll "\${OUT}/win_\${arch}/"
  cp Qt5Widgets.dll "\${OUT}/win_\${arch}/"
  cp zlib1.dll "\${OUT}/win_\${arch}/"
  cp liblzma-5.dll "\${OUT}/win_\${arch}/"
  cp libbz2-1.dll "\${OUT}/win_\${arch}/"
  cp libintl-8.dll "\${OUT}/win_\${arch}/"

  cp qcma.exe "\${OUT}/win_\${arch}/"
  cp qcma_console.exe "\${OUT}/win_\${arch}/"

  pushd /cygdrive/c/msys64/mingw\${bits}/share/qt5/plugins
  mkdir "\${OUT}/win_\${arch}/platforms"
  mkdir "\${OUT}/win_\${arch}/imageformats"
  mkdir "\${OUT}/win_\${arch}/sqldrivers"
  cp platforms/qwindows.dll "\${OUT}/win_\${arch}/platforms"
  cp imageformats/qgif.dll "\${OUT}/win_\${arch}/imageformats"
  cp imageformats/qjpeg.dll "\${OUT}/win_\${arch}/imageformats"
  cp imageformats/qtiff.dll "\${OUT}/win_\${arch}/imageformats"
  cp sqldrivers/qsqlite.dll "\${OUT}/win_\${arch}/sqldrivers"

  pushd /cygdrive/c/msys64/mingw\${bits}/share/qt5/translations
  mkdir "\${OUT}/win_\${arch}/"translations
  cp qt_*.qm "\${OUT}/win_\${arch}/"translations
  popd
  popd
  popd
  popd
done
"/cygdrive/c/Program Files (x86)/NSIS/makensis" qcma.nsi
EOSSH
scp ${SERVER_HOST}:Qcma_setup.exe Qcma_setup-${VERSION}.exe
