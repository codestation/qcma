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

From the qcma repository, build it with:

```sh
buildscripts/appimage/build.sh
```

The wrapper maps the host UID/GID into the image, mounts both repositories
read-only and writes the result to `artifacts/appimage/x86_64`. Set
`VITAMTP_SOURCE` or pass an alternate output directory when needed.
