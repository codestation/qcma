#!/bin/sh
set -eu

qcma_source=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
vitamtp_source=${VITAMTP_SOURCE:-"$(dirname "$qcma_source")/vitamtp"}
obs_checkout=${1:-${OBS_CHECKOUT:-"$HOME/obs/home:codestation"}}

qcma_version=$(sed -n 's/^project(qcma VERSION \([^ ]*\).*/\1/p' "$qcma_source/CMakeLists.txt")
vitamtp_version=$(sed -n 's/^AC_INIT(\[vitamtp\], \[\([^]]*\)\].*/\1/p' "$vitamtp_source/configure.ac")

test -d "$obs_checkout/qcma/.osc"
test -d "$obs_checkout/vitamtp/.osc"

"$vitamtp_source/buildscripts/release_to_obs.sh" "$vitamtp_version" "$obs_checkout/vitamtp"
"$qcma_source/buildscripts/obs/release-qcma.sh" "$qcma_version" "$obs_checkout/qcma"

(cd "$obs_checkout/vitamtp" && osc addremove && osc status)
(cd "$obs_checkout/qcma" && osc addremove && osc status)

echo "OBS checkout prepared. No osc commit was run."
