#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
output=${1:-"$root/artifacts/opensuse-tumbleweed/x86_64"}
"$root/buildscripts/common/run-container-build.sh" rpm opensuse-tumbleweed linux/amd64 \
    "$root/buildscripts/opensuse/Dockerfile.tumbleweed" "$output"
