# Qcma AppImage

OBS builds `appimage.yml` from the qcma and libvitamtp RPMs in the same
project. Enable an x86_64 AppImage repository whose first path is the qcma
RPM repository and whose second path is `OBS:AppImage/AppImage`.

The resulting AppImage cannot install host udev rules. Copy
`60-psvita.rules` from the extracted AppImage to `/usr/lib/udev/rules.d/`
and reload udev rules before using USB as a regular user.

The included `Dockerfile.ubuntu24` is the self-contained reference build
environment. It downloads pinned, SHA-256-verified x86_64 releases of
linuxdeploy and linuxdeploy-plugin-qt, so neither tool is required on the host.
Building on the oldest supported distribution keeps the resulting glibc
requirement portable.

From the qcma repository, build and run it with:

```sh
docker build -t qcma-appimage:ubuntu24 -f buildscripts/appimage/Dockerfile.ubuntu24 .
mkdir -p /tmp/qcma-appimage-output
docker run --rm \
  -v "$PWD:/qcma:ro" \
  -v "/path/to/vitamtp:/vitamtp:ro" \
  -v "/tmp/qcma-appimage-output:/out" \
  qcma-appimage:ubuntu24 sh -c \
  'git config --global --add safe.directory /qcma && \
   git config --global --add safe.directory /vitamtp && \
   /qcma/buildscripts/appimage/build-appimage.sh \
     /vitamtp /out/qcma-0.5.2-x86_64.AppImage'
```
