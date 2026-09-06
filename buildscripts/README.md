# Packaging and release builds

Each directory is an entry point for one package family or operating system:

- `debian`: Debian 13 packages for amd64, armhf and arm64.
- `ubuntu`: Ubuntu 24.04 and 26.04 amd64 packages.
- `fedora`: Fedora 44 x86_64 and aarch64 RPMs.
- `opensuse`: openSUSE Tumbleweed x86_64 RPMs.
- `appimage`: self-contained x86_64 AppImage build.
- `windows`: native Windows x64 installer using Visual Studio 2022, vcpkg, Qt 6 for MSVC and NSIS.
- `macos`: native macOS 13+ Apple Silicon DMG using Homebrew and Qt 6.
- `obs`: reproducible source preparation for the local OBS checkout.
- `common`: shared implementation used by distribution wrappers.

Run `build-all.sh [output]` for the full Linux package matrix. It builds
vitamtp before qcma and never publishes artifacts. Raspbian remains an OBS
target; Debian 13 armhf/arm64 provide its local architecture preflight.
The Linux packaging containers run as root so they can install and exercise
the packages they produce. Git archives are created as the unprivileged
`builder` user, and artifacts are returned to the invoking host UID/GID.

Windows and macOS are intentionally excluded from `build-all.sh`. Their
`prepare` and `build` entry points run directly on native hosts, build VitaMTP
first and write unsigned release artifacts under `artifacts/`.
