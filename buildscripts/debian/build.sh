#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
requested=${1:-all}
base_output=${2:-"$root/artifacts/debian-13"}

build_arch() {
    case "$1" in
        amd64|x86_64) platform=linux/amd64; output_arch=amd64 ;;
        arm64|aarch64) platform=linux/arm64; output_arch=arm64 ;;
        armv7|armv7l|armhf) platform=linux/arm/v7; output_arch=armhf ;;
        *) echo "Unsupported Debian architecture: $1" >&2; exit 2 ;;
    esac
    "$root/buildscripts/common/run-container-build.sh" deb "debian-13" "$platform" \
        "$root/buildscripts/debian/Dockerfile.13" "$base_output/$output_arch"
}

if [ "$requested" = all ]; then
    for arch in amd64 arm64 armhf; do build_arch "$arch"; done
else
    build_arch "$requested"
fi
