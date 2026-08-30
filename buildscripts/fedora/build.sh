#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
requested=${1:-all}
base_output=${2:-"$root/artifacts/fedora-44"}

build_arch() {
    case "$1" in
        amd64|x86_64) platform=linux/amd64; output_arch=x86_64 ;;
        arm64|aarch64) platform=linux/arm64; output_arch=aarch64 ;;
        *) echo "Unsupported Fedora architecture: $1" >&2; exit 2 ;;
    esac
    "$root/buildscripts/common/run-container-build.sh" rpm fedora-44 "$platform" \
        "$root/buildscripts/fedora/Dockerfile.44" "$base_output/$output_arch"
}

if [ "$requested" = all ]; then
    for arch in x86_64 aarch64; do build_arch "$arch"; done
else
    build_arch "$requested"
fi
