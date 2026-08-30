#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
requested=${1:-all}
base_output=${2:-"$root/artifacts"}

build_release() {
    case "$1" in 24.04|26.04) ;; *) echo "Unsupported Ubuntu release: $1" >&2; exit 2 ;; esac
    "$root/buildscripts/common/run-container-build.sh" deb "ubuntu-$1" linux/amd64 \
        "$root/buildscripts/ubuntu/Dockerfile.$1" "$base_output/ubuntu-$1/amd64"
}

if [ "$requested" = all ]; then
    build_release 24.04
    build_release 26.04
else
    build_release "$requested"
fi
