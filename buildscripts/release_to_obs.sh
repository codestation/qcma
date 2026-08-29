#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_dir"

version=${1:-}
obs_package=${2:-}

if [ -z "$version" ] || [ -z "$obs_package" ]; then
    echo "Usage: $0 <version> <OBS qcma package directory>" >&2
    exit 2
fi

expected=$(sed -n 's/^VERSION = //p' config.pri)
if [ "$version" != "$expected" ]; then
    echo "Version mismatch: config.pri contains $expected" >&2
    exit 1
fi
if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "Refusing to create release artifacts from a dirty tree" >&2
    exit 1
fi

epoch=$(git log -1 --format=%ct)
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

archive="qcma-$version.tar.gz"
git archive --format=tar --prefix="qcma-$version/" HEAD | gzip -n -9 >"$tmpdir/$archive"

mkdir "$tmpdir/debian"
cp -a debian/. "$tmpdir/debian/"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner \
    -C "$tmpdir" -cJf "$tmpdir/debian.tar.xz" debian

size=$(wc -c <"$tmpdir/$archive" | tr -d ' ')
sha1=$(sha1sum "$tmpdir/$archive" | cut -d' ' -f1)
sha256=$(sha256sum "$tmpdir/$archive" | cut -d' ' -f1)
md5=$(md5sum "$tmpdir/$archive" | cut -d' ' -f1)

sed -e "s/^Version:.*/Version: $version/" \
    -e "s/^Build-Depends:.*libvitamtp-dev (>= [^)]*)/Build-Depends: debhelper-compat (= 13), libavcodec-dev, libavformat-dev, libswscale-dev, qmake6, qt6-base-dev, qt6-base-dev-tools, qt6-l10n-tools, libnotify-dev, pkg-config, libvitamtp-dev (>= 2.5.11)/" \
    buildscripts/debian/qcma.dsc.in >"$tmpdir/qcma.dsc"
cat >>"$tmpdir/qcma.dsc" <<EOF
Checksums-Sha1:
 $sha1 $size $archive
Checksums-Sha256:
 $sha256 $size $archive
Files:
 $md5 $size $archive
EOF

install -m644 "$tmpdir/$archive" "$obs_package/$archive"
install -m644 "$tmpdir/debian.tar.xz" "$obs_package/debian.tar.xz"
install -m644 "$tmpdir/qcma.dsc" "$obs_package/qcma.dsc"
install -m644 buildscripts/rpmbuild/qcma.spec "$obs_package/qcma.spec"
install -m644 buildscripts/rpmbuild/qcma.changes "$obs_package/qcma.changes"
install -m644 buildscripts/appimage/appimage.yml "$obs_package/appimage.yml"
install -m644 buildscripts/appimage/_service "$obs_package/_service"

echo "Prepared qcma $version in $obs_package"
