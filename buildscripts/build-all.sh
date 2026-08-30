#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output=${1:-"$root/artifacts"}

"$root/buildscripts/debian/build.sh" all "$output/debian-13"
"$root/buildscripts/ubuntu/build.sh" all "$output"
"$root/buildscripts/fedora/build.sh" all "$output/fedora-44"
"$root/buildscripts/opensuse/build.sh" "$output/opensuse-tumbleweed/x86_64"
