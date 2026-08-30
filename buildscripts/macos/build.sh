#!/bin/bash

set -euo pipefail

usage() {
    cat <<'EOF'
usage: build.sh [--vitamtp-source PATH] [--output-dir PATH]
EOF
}

script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
qcma_source=$(CDPATH='' cd -- "$script_dir/../.." && pwd)
if [[ -d $qcma_source/../vitamtp ]]; then
    vitamtp_source=$(CDPATH='' cd -- "$qcma_source/../vitamtp" && pwd)
else
    vitamtp_source=
fi
output=$qcma_source/artifacts/macos

while [[ $# -gt 0 ]]; do
    case $1 in
        --vitamtp-source)
            [[ $# -ge 2 ]] || { usage >&2; exit 2; }
            vitamtp_source=$(CDPATH='' cd -- "$2" && pwd)
            shift 2
            ;;
        --output-dir)
            [[ $# -ge 2 ]] || { usage >&2; exit 2; }
            mkdir -p -- "$2"
            output=$(CDPATH='' cd -- "$2" && pwd)
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

[[ $(uname -s) == Darwin ]] || { echo 'This script must run on macOS.' >&2; exit 1; }
[[ $(uname -m) == arm64 ]] || { echo 'This build supports Apple Silicon (arm64) only.' >&2; exit 1; }
[[ -n $vitamtp_source && -f $vitamtp_source/configure.ac ]] || {
    echo 'VitaMTP was not found. Place it beside qcma or pass --vitamtp-source.' >&2
    exit 1
}
command -v brew >/dev/null 2>&1 || { echo 'Homebrew is missing; run prepare.sh first.' >&2; exit 1; }

for tool in autoreconf cmake create-dmg dylibbundler ninja pkg-config; do
    command -v "$tool" >/dev/null 2>&1 || { echo "missing build tool: $tool; run prepare.sh" >&2; exit 1; }
done

qt_prefix=$(brew --prefix qt)
macdeployqt_bin=$qt_prefix/bin/macdeployqt
[[ -x $macdeployqt_bin ]] || { echo "missing Qt tool: $macdeployqt_bin" >&2; exit 1; }

mkdir -p -- "$output"
work=$output/.work
vitamtp_build=$work/vitamtp-build
vitamtp_work_source=$work/vitamtp-source
vitamtp_prefix=$work/vitamtp-prefix
qcma_build=$work/qcma-build
stage=$work/dmg-root
app=$stage/Qcma.app

rm -rf -- "$vitamtp_build" "$vitamtp_work_source" "$vitamtp_prefix" "$qcma_build" "$stage"
mkdir -p -- "$vitamtp_build" "$vitamtp_work_source" "$vitamtp_prefix" "$qcma_build" "$stage"

version=$(git -C "$qcma_source" describe --tags --always --dirty 2>/dev/null || true)
version=${version#v}
if [[ -z $version ]]; then
    version=$(sed -n 's/^project(qcma VERSION \([^ ]*\).*/\1/p' "$qcma_source/CMakeLists.txt")
fi
version=${version//\//-}
dmg=$output/Qcma-$version-macos-arm64.dmg

gettext_prefix=$(brew --prefix gettext)
libtool_prefix=$(brew --prefix libtool)
libusb_prefix=$(brew --prefix libusb)
libxml2_prefix=$(brew --prefix libxml2)
ffmpeg_prefix=$(brew --prefix ffmpeg)

export PATH="$qt_prefix/bin:$gettext_prefix/bin:$libtool_prefix/bin:$PATH"
export MACOSX_DEPLOYMENT_TARGET=13.0
export PKG_CONFIG_PATH="$vitamtp_prefix/lib/pkgconfig:$libusb_prefix/lib/pkgconfig:$libxml2_prefix/lib/pkgconfig:$ffmpeg_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

echo "Building on macOS $(sw_vers -productVersion) arm64 with Qt 6"
brew list --versions qt ffmpeg libusb libxml2 dylibbundler create-dmg

rsync -a --exclude=.git --exclude=autom4te.cache "$vitamtp_source/" "$vitamtp_work_source/"
(
    cd "$vitamtp_work_source"
    autoreconf -fi
)
(
    cd "$vitamtp_build"
    "$vitamtp_work_source/configure" --prefix="$vitamtp_prefix" --disable-static
    make -j"$(sysctl -n hw.logicalcpu)"
    make install
)

cmake -S "$qcma_source" -B "$qcma_build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 -DBUILD_TESTING=ON
cmake --build "$qcma_build"
ctest --test-dir "$qcma_build" --output-on-failure

built_app=$(find "$qcma_build" -maxdepth 4 -type d -iname 'qcma.app' -print -quit)
[[ -n $built_app ]] || { echo 'Qcma.app was not produced.' >&2; exit 1; }
ditto "$built_app" "$app"

app_executable=$(find "$app/Contents/MacOS" -maxdepth 1 -type f -perm -111 -print -quit)
[[ -n $app_executable ]] || { echo 'The application bundle has no executable.' >&2; exit 1; }

# Qt deployment intentionally runs without signing because dylibbundler changes
# load commands afterwards. A final ad-hoc signature covers the complete app.
"$macdeployqt_bin" "$app" -always-overwrite -no-codesign -verbose=2

search_args=(
    -s "$vitamtp_prefix/lib"
    -s "$libusb_prefix/lib"
    -s "$libxml2_prefix/lib"
    -s "$ffmpeg_prefix/lib"
    -s "$(brew --prefix)/lib"
)
dylibbundler -od -b -x "$app_executable" \
    -d "$app/Contents/libs" \
    -p '@executable_path/../libs/' \
    "${search_args[@]}"

codesign --force --deep --sign - --timestamp=none "$app"
"$script_dir/verify-bundle.sh" "$app"

rm -f -- "$dmg"
create-dmg \
    --volname "Qcma $version" \
    --volicon "$script_dir/installer.icns" \
    --background "$script_dir/installer-bg.png" \
    --window-size 640 480 \
    --icon-size 80 \
    --icon 'Qcma.app' 182 344 \
    --app-drop-link 456 344 \
    "$dmg" "$stage"
hdiutil verify "$dmg"

echo "Created $dmg"
