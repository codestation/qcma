#!/bin/sh
set -eu

echo "Registering arm64 and arm/v7 QEMU handlers requires a privileged container."
docker run --privileged --rm tonistiigi/binfmt --install arm64,arm
docker buildx inspect --bootstrap
