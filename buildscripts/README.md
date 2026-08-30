# Packaging and release builds

Each directory is an entry point for one package family or operating system:

- `debian`: Debian 13 packages for amd64, armhf and arm64.
- `ubuntu`: Ubuntu 24.04 and 26.04 amd64 packages.
- `fedora`: Fedora 44 x86_64 and aarch64 RPMs.
- `opensuse`: openSUSE Tumbleweed x86_64 RPMs.
- `appimage`: self-contained x86_64 AppImage build.
- `windows` and `macos`: legacy native release scripts, pending Qt 6 modernization.
- `obs`: reproducible source preparation for the local OBS checkout.
- `common`: shared implementation used by distribution wrappers.

Run `build-all.sh [output]` for the full Linux package matrix. It builds
vitamtp before qcma and never publishes artifacts. Raspbian remains an OBS
target; Debian 13 armhf/arm64 provide its local architecture preflight.
All Linux containers build as a non-root user whose UID/GID match the host;
root is used only through `sudo` for package installation tests inside the
throwaway package containers.
