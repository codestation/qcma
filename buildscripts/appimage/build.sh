#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
vitamtp_source=${VITAMTP_SOURCE:-"$(dirname "$root")/vitamtp"}
output_dir=${1:-"$root/artifacts/appimage/x86_64"}
version=$(sed -n 's/^VERSION = //p' "$root/config.pri")

test -d "$vitamtp_source/.git"
git -C "$root" diff --quiet
git -C "$root" diff --cached --quiet
git -C "$vitamtp_source" diff --quiet
git -C "$vitamtp_source" diff --cached --quiet
mkdir -p "$output_dir"
output_dir=$(readlink -f "$output_dir")

docker build --build-arg "HOST_UID=$(id -u)" --build-arg "HOST_GID=$(id -g)" \
    -t qcma-appimage:ubuntu24 -f "$root/buildscripts/appimage/Dockerfile.ubuntu24" "$root"
docker run --rm \
    -v "$root:/qcma:ro" \
    -v "$(readlink -f "$vitamtp_source"):/vitamtp:ro" \
    -v "$output_dir:/out" \
    qcma-appimage:ubuntu24 \
    /qcma/buildscripts/appimage/build-appimage.sh \
    /vitamtp "/out/qcma-${version}-x86_64.AppImage"
