#!/bin/sh

[ $# -eq 0 ] && { echo "Usage: $0 <version>"; exit 1; }

sed -i "s/Version:.*/Version:        $1/" rpmbuild/qcma-fedora.spec
sed -i "s/Version:.*/Version:        $1/" rpmbuild/qcma-openSUSE.spec
sed -i "s/VERSION = .*/VERSION = $1/"     qcma_common.pri
echo "Don't forget to update the changelog"
