#!/bin/sh
set -eu

qcma_source=${1:?qcma source directory is required}
vitamtp_source=${2:?vitamtp source directory is required}
output=${3:?output directory is required}

qcma_source=$(readlink -f "$qcma_source")
vitamtp_source=$(readlink -f "$vitamtp_source")
output=$(readlink -m "$output")
work=$(mktemp -d)
cleanup() {
    rm -rf "$work"
    chown -R "${BUILD_OUTPUT_UID:?}:${BUILD_OUTPUT_GID:?}" "$output"
}
trap cleanup EXIT HUP INT TERM
topdir="$work/rpmbuild"
mkdir -p "$topdir"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS} "$output"
build_user=$(getent passwd "${BUILD_OUTPUT_UID:?}" | cut -d: -f1)
[ -n "$build_user" ] || { echo "No build user exists for UID $BUILD_OUTPUT_UID" >&2; exit 1; }

vitamtp_version=$(sed -n 's/^AC_INIT(\[vitamtp\], \[\([^]]*\)\].*/\1/p' "$vitamtp_source/configure.ac")
qcma_version=$(sed -n 's/^project(qcma VERSION \([^ ]*\).*/\1/p' "$qcma_source/CMakeLists.txt")

runuser -u "$build_user" -- git -C "$vitamtp_source" archive --format=tar --prefix="vitamtp-${vitamtp_version}/" HEAD |
    gzip -n -9 >"$topdir/SOURCES/vitamtp-${vitamtp_version}.tar.gz"
runuser -u "$build_user" -- git -C "$qcma_source" archive --format=tar --prefix="qcma-${qcma_version}/" HEAD |
    gzip -n -9 >"$topdir/SOURCES/qcma-${qcma_version}.tar.gz"
cp "$vitamtp_source/rpmbuild/vitamtp.spec" "$topdir/SPECS/"
cp "$qcma_source/buildscripts/common/rpm/qcma.spec" "$topdir/SPECS/"

rpmbuild --define "_topdir $topdir" -ba "$topdir/SPECS/vitamtp.spec"

. /etc/os-release
case "$ID" in
    fedora)
        dnf install -y "$topdir"/RPMS/*/libvitamtp5-*.rpm "$topdir"/RPMS/*/libvitamtp-devel-*.rpm
        ;;
    opensuse-tumbleweed|opensuse-leap|opensuse)
        zypper --non-interactive install --allow-unsigned-rpm \
            "$topdir"/RPMS/*/libvitamtp5-*.rpm "$topdir"/RPMS/*/libvitamtp-devel-*.rpm
        ;;
    *)
        echo "Unsupported RPM distribution: $ID" >&2
        exit 1
        ;;
esac

rpmbuild --define "_topdir $topdir" -ba "$topdir/SPECS/qcma.spec"
find "$topdir/RPMS" "$topdir/SRPMS" -type f -name '*.rpm' -exec cp -a {} "$output/" \;
rpm -qpi "$output"/qcma-${qcma_version}-*.rpm >/dev/null
rpm -qpi "$output"/qcma-cli-${qcma_version}-*.rpm >/dev/null
case "$ID" in
    fedora)
        dnf install -y "$topdir"/RPMS/*/qcma-${qcma_version}-*.rpm \
            "$topdir"/RPMS/*/qcma-cli-${qcma_version}-*.rpm
        ;;
    *)
        zypper --non-interactive install --allow-unsigned-rpm \
            "$topdir"/RPMS/*/qcma-${qcma_version}-*.rpm \
            "$topdir"/RPMS/*/qcma-cli-${qcma_version}-*.rpm
        ;;
esac
/usr/bin/qcma --version | grep -Fx "qcma $qcma_version"
/usr/bin/qcma_cli --version | grep -Fx "qcma_cli $qcma_version"
if ldd /usr/bin/qcma /usr/bin/qcma_cli | grep -q 'not found'; then
    echo "Installed RPM packages contain unresolved libraries" >&2
    exit 1
fi
rpmlint "$output"/*.rpm || echo "rpmlint reported packaging policy findings" >&2
echo "RPM packages created in $output"
