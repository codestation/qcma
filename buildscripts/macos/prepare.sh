#!/bin/sh

set -eu

script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)

if [ "$(uname -s)" != Darwin ]; then
    echo 'This script must run on macOS.' >&2
    exit 1
fi
if [ "$(uname -m)" != arm64 ]; then
    echo 'The supported macOS build host is Apple Silicon (arm64).' >&2
    exit 1
fi
if ! command -v brew >/dev/null 2>&1; then
    echo 'Homebrew is required. Install it from https://brew.sh and run this script again.' >&2
    exit 1
fi

brew update
brew bundle --file "$script_dir/Brewfile"

echo 'macOS build environment is ready.'
echo 'Run buildscripts/macos/build.sh to build Qcma.'
