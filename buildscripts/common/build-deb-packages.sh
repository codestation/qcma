#!/bin/sh
set -eu

qcma_source=${1:?qcma source directory is required}
vitamtp_source=${2:?vitamtp source directory is required}
output=${3:?output directory is required}

qcma_source=$(readlink -f "$qcma_source")
vitamtp_source=$(readlink -f "$vitamtp_source")
output=$(readlink -m "$output")
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$output"

export DEB_BUILD_OPTIONS=${DEB_BUILD_OPTIONS:-parallel=$(nproc)}

build_source() {
    name=$1
    version=$2
    source=$3
    source_dir="$work/${name}-${version}"
    mkdir "$source_dir"
    git -C "$source" archive HEAD | tar -x -C "$source_dir"
    git -C "$source" archive --format=tar --prefix="${name}-${version}/" HEAD |
        gzip -n -9 >"$work/${name}_${version}.orig.tar.gz"

    cd "$source_dir"
    dpkg-buildpackage --no-sign -S
    dpkg-buildpackage --no-sign -b
}

vitamtp_version=$(sed -n 's/^AC_INIT(\[vitamtp\], \[\([^]]*\)\].*/\1/p' "$vitamtp_source/configure.ac")
qcma_version=$(sed -n 's/^VERSION = //p' "$qcma_source/config.pri")

build_source vitamtp "$vitamtp_version" "$vitamtp_source"
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y "$work/libvitamtp5_${vitamtp_version}"*_$(dpkg --print-architecture).deb \
    "$work/libvitamtp-dev_${vitamtp_version}"*_$(dpkg --print-architecture).deb
build_source qcma "$qcma_version" "$qcma_source"

find "$work" -maxdepth 1 -type f \
    \( -name '*.deb' -o -name '*.dsc' -o -name '*.changes' -o \
       -name '*.buildinfo' -o -name '*.tar.*' \) -exec cp -a {} "$output/" \;

dpkg-deb --info "$output/qcma_${qcma_version}"*_$(dpkg --print-architecture).deb >/dev/null
dpkg-deb --info "$output/qcma-cli_${qcma_version}"*_$(dpkg --print-architecture).deb >/dev/null
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y "$output/qcma_${qcma_version}"*_$(dpkg --print-architecture).deb \
    "$output/qcma-cli_${qcma_version}"*_$(dpkg --print-architecture).deb
if ldd /usr/bin/qcma /usr/bin/qcma_cli | grep -q 'not found'; then
    echo "Installed Debian packages contain unresolved libraries" >&2
    exit 1
fi
lintian --fail-on error "$output"/*.deb "$output"/*.dsc
echo "Debian packages created in $output"
