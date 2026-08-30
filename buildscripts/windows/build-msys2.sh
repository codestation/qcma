#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 4 ]]; then
    echo "usage: $0 <qcma-source> <vitamtp-source> <output> <enabled|disabled>" >&2
    exit 2
fi

qcma_source=$1
vitamtp_source=$2
output=$3
ffmpeg_mode=$4

if [[ $ffmpeg_mode != enabled && $ffmpeg_mode != disabled ]]; then
    echo "invalid FFmpeg mode: $ffmpeg_mode" >&2
    exit 2
fi

export PATH="/ucrt64/bin:/usr/bin:$PATH"
for tool in autoreconf make pkg-config ntldd objdump; do
    command -v "$tool" >/dev/null || { echo "missing build tool: $tool" >&2; exit 1; }
done

find_tool() {
    local candidate
    for candidate in "$@"; do
        if command -v "$candidate" >/dev/null 2>&1; then
            command -v "$candidate"
            return 0
        fi
    done
    return 1
}

qmake_bin=$(find_tool qmake6 qmake-qt6 qmake) || { echo 'Qt 6 qmake was not found' >&2; exit 1; }
lrelease_bin=$(find_tool lrelease6 lrelease-qt6 lrelease) || { echo 'Qt 6 lrelease was not found' >&2; exit 1; }
windeployqt_bin=$(find_tool windeployqt6 windeployqt-qt6 windeployqt) || { echo 'windeployqt was not found' >&2; exit 1; }

qt_version=$($qmake_bin -query QT_VERSION)
[[ $qt_version == 6.* ]] || { echo "Qt 6 is required, found $qt_version" >&2; exit 1; }

work=$output/.work
vitamtp_build=$work/vitamtp-build
vitamtp_work_source=$work/vitamtp-source
vitamtp_prefix=$work/vitamtp-prefix
qcma_build=$work/qcma-build
payload=$work/payload

rm -rf -- "$vitamtp_build" "$vitamtp_work_source" "$vitamtp_prefix" "$qcma_build" "$payload"
mkdir -p -- "$vitamtp_build" "$vitamtp_work_source" "$vitamtp_prefix" "$qcma_build" "$payload"

version=$(git -C "$qcma_source" describe --tags --always --dirty 2>/dev/null || true)
version=${version#v}
if [[ -z $version ]]; then
    version=$(sed -n 's/^VERSION[[:space:]]*=[[:space:]]*//p' "$qcma_source/qcma.pro" | head -n1)
fi
version=${version//\//-}
printf '%s\n' "$version" > "$work/version.txt"

echo "Building with MSYS2 $(uname -sr), Qt $qt_version and $($qmake_bin -query QMAKE_XSPEC)"
pacman -Q | grep -E '^(mingw-w64-ucrt-x86_64-(qt6|ffmpeg|libusb|libxml2)|gcc|make) ' || true

cp -a "$vitamtp_source/." "$vitamtp_work_source/"
rm -rf -- "$vitamtp_work_source/.git" "$vitamtp_work_source/autom4te.cache"
(
    cd "$vitamtp_work_source"
    autoreconf -fi
)
(
    cd "$vitamtp_build"
    "$vitamtp_work_source/configure" \
        --host=x86_64-w64-mingw32 \
        --prefix="$vitamtp_prefix" \
        --disable-static
    make -j"${NUMBER_OF_PROCESSORS:-2}"
    make install
)

export PKG_CONFIG_PATH="$vitamtp_prefix/lib/pkgconfig:/ucrt64/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
"$lrelease_bin" "$qcma_source/qcma.pro"

qmake_args=("$qcma_source/qcma.pro" 'CONFIG+=release')
if [[ $ffmpeg_mode == disabled ]]; then
    qmake_args+=('CONFIG+=DISABLE_FFMPEG')
fi
(
    cd "$qcma_build"
    "$qmake_bin" "${qmake_args[@]}"
    make -j"${NUMBER_OF_PROCESSORS:-2}"
)

qcma_exe=$(find "$qcma_build/gui" -type f -iname qcma.exe -print -quit)
[[ -n $qcma_exe ]] || { echo 'qcma.exe was not produced' >&2; exit 1; }
install -m755 "$qcma_exe" "$payload/qcma.exe"
install -m644 "$qcma_source/COPYING" "$payload/COPYING.txt"
install -m644 "$qcma_source/gui/resources/images/qcma.ico" "$payload/qcma.ico"

vitamtp_dll=$(find "$vitamtp_prefix/bin" -type f -iname 'libvitamtp-*.dll' -print -quit)
[[ -n $vitamtp_dll ]] || { echo 'VitaMTP DLL was not produced' >&2; exit 1; }
install -m755 "$vitamtp_dll" "$payload/$(basename "$vitamtp_dll")"

"$windeployqt_bin" --release "$payload/qcma.exe"

# windeployqt only handles Qt. ntldd resolves the complete MinGW dependency
# closure, including VitaMTP, FFmpeg, libusb and libxml2.
mapfile -t runtime_dlls < <(
    ntldd -R "$payload/qcma.exe" 2>&1 |
        sed 's#\\\\#/#g' |
        sed -nE 's#.*[/[:space:]]([^/[:space:]]+\.dll).*#\1#Ip' |
        sort -fu
)
for dll_name in "${runtime_dlls[@]}"; do
    dll=$(find /ucrt64/bin -maxdepth 1 -type f -iname "$dll_name" -print -quit)
    if [[ -n $dll ]]; then
        install -m755 "$dll" "$payload/$(basename "$dll")"
    fi
done

dependency_report=$(PATH="$payload:/c/Windows/System32:/c/Windows" /ucrt64/bin/ntldd -R "$payload/qcma.exe" 2>&1 || true)
printf '%s\n' "$dependency_report" > "$work/ntldd.txt"
if grep -qiE 'not found|could not find' "$work/ntldd.txt"; then
    cat "$work/ntldd.txt" >&2
    echo 'The Windows payload has unresolved DLL dependencies' >&2
    exit 1
fi

objdump -f "$payload/qcma.exe" | grep -q 'pei-x86-64' || {
    echo 'qcma.exe is not an x86_64 PE executable' >&2
    exit 1
}

echo "Prepared Windows x86_64 payload for Qcma $version"
