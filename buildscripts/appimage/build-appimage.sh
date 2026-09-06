#!/bin/sh
set -eu

vitamtp_source=${1:-}
output=${2:-qcma-0.5.4-x86_64.AppImage}
linuxdeploy=${LINUXDEPLOY:-/opt/linuxdeploy/linuxdeploy-x86_64.AppImage}
qt_plugin=${LINUXDEPLOY_PLUGIN_QT:-/opt/linuxdeploy/linuxdeploy-plugin-qt-x86_64.AppImage}
if [ -z "$vitamtp_source" ]; then
    echo "Usage: $0 <vitamtp source> [output]" >&2
    exit 2
fi

qcma_source=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
vitamtp_source=$(CDPATH= cd -- "$vitamtp_source" && pwd)
linuxdeploy=$(readlink -f "$linuxdeploy")
qt_plugin=$(readlink -f "$qt_plugin")
output=$(readlink -m "$output")
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT HUP INT TERM

mkdir "$work_dir/vitamtp" "$work_dir/qcma"
git -C "$vitamtp_source" archive HEAD | tar -x -C "$work_dir/vitamtp"
git -C "$qcma_source" archive HEAD | tar -x -C "$work_dir/qcma"
prefix="$work_dir/prefix"
appdir="$work_dir/AppDir"
mkdir -p "$prefix" "$appdir/usr/lib" "$appdir/usr/share/metainfo"

cmake -S "$work_dir/vitamtp" -B "$work_dir/vitamtp-build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" -DBUILD_TESTING=ON
cmake --build "$work_dir/vitamtp-build"
ctest --test-dir "$work_dir/vitamtp-build" --output-on-failure
cmake --install "$work_dir/vitamtp-build"

cd "$work_dir/qcma"
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$prefix" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=ON
PKG_CONFIG_PATH="$prefix/lib/pkgconfig" cmake --build build
PKG_CONFIG_PATH="$prefix/lib/pkgconfig" ctest --test-dir build --output-on-failure
DESTDIR="$appdir" cmake --install build

cp -a "$prefix/lib/libvitamtp.so."* "$appdir/usr/lib/"
usb_libdir=$(pkg-config --variable=libdir libusb-1.0)
cp -a "$usb_libdir/libusb-1.0.so.0"* "$appdir/usr/lib/"
install -Dm644 dev.megpoid.Qcma.metainfo.xml "$appdir/usr/share/metainfo/dev.megpoid.Qcma.appdata.xml"
mv "$appdir/usr/share/applications/qcma.desktop" \
    "$appdir/usr/share/applications/dev.megpoid.Qcma.desktop"
install -Dm644 COPYING "$appdir/usr/share/licenses/qcma/COPYING"
install -Dm644 "$vitamtp_source/debian/libvitamtp5.udev" "$appdir/usr/share/doc/qcma/60-psvita.rules"

test -x "$linuxdeploy"
test -x "$qt_plugin"
tool_path=$(dirname "$qt_plugin")

cd "$work_dir"
PATH="$tool_path:$PATH" APPIMAGE_EXTRACT_AND_RUN=1 \
LD_LIBRARY_PATH="$prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
LDAI_OUTPUT="$output" "$linuxdeploy" --appdir "$appdir" \
    --desktop-file "$appdir/usr/share/applications/dev.megpoid.Qcma.desktop" \
    --icon-file "$appdir/usr/share/icons/hicolor/64x64/apps/qcma.png" \
    --plugin qt --output appimage

test -x "$output"
APPIMAGE_EXTRACT_AND_RUN=1 "$output" --version | grep -Fx 'qcma 0.5.4'
echo "Created $output"
