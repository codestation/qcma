#!/bin/sh
set -eu

kind=${1:?package kind is required}
target=${2:?target name is required}
platform=${3:?Docker platform is required}
dockerfile=${4:?Dockerfile is required}
output=${5:?output directory is required}

qcma_source=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
vitamtp_source=${VITAMTP_SOURCE:-"$(dirname "$qcma_source")/vitamtp"}
vitamtp_source=$(readlink -f "$vitamtp_source")
dockerfile=$(readlink -f "$dockerfile")
mkdir -p "$output"
output=$(readlink -f "$output")

test -d "$vitamtp_source/.git"
git -C "$qcma_source" diff --quiet
git -C "$qcma_source" diff --cached --quiet
git -C "$vitamtp_source" diff --quiet
git -C "$vitamtp_source" diff --cached --quiet

image="qcma-package-${target}-$(printf '%s' "$platform" | tr '/:' '--')"
docker buildx build --platform "$platform" --load \
    --build-arg "HOST_UID=$(id -u)" --build-arg "HOST_GID=$(id -g)" \
    -t "$image" -f "$dockerfile" "$qcma_source"
docker run --rm --platform "$platform" \
    -v "$qcma_source:/qcma:ro" \
    -v "$vitamtp_source:/vitamtp:ro" \
    -v "$output:/out" \
    "$image" "/qcma/buildscripts/common/build-${kind}-packages.sh" /qcma /vitamtp /out
