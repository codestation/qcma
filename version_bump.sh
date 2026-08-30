#!/bin/sh
set -eu

[ $# -eq 0 ] && { echo "Usage: $0 <version>"; exit 1; }

version=$1
sed -i "s/%define _version.*/%define _version ${version}/" buildscripts/common/rpm/qcma.spec
sed -i "s/project(qcma VERSION [^ ]*/project(qcma VERSION ${version}/" CMakeLists.txt
echo "Remember to update debian/changelog, AppStream metadata and release manifests."
