# Windows x64 build

Qcma's supported Windows toolchain is Visual Studio 2022 (MSVC), vcpkg and
Qt 6 MSVC 2022 64-bit and libwdi. MinGW and MSYS2 are not used.

Run `prepare.ps1` from an elevated PowerShell to install/check Visual Studio
Build Tools, Redistributable WDK 8 Components, CMake, Git, NSIS 3.12 or later,
Python, Qt and vcpkg. If PowerShell blocks local scripts, allow them for the
current session only with:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

Qt is installed unattended with `aqtinstall`, which downloads the public
Qt 6.10.2 MSVC 2022 x64 packages and does not require a Qt Account. The 
Visual C++ Build Tools workload is installed explicitly. Then build with:

```powershell
.\build.ps1
```

The `qcma-winusb.exe` helper is built from libwdi with WinUSB x64 support. It
requires the [Redistributable WDK 8 Components](https://github.com/pbatard/libwdi/wiki/Install)
at `C:\Program Files (x86)\Windows Kits\8.0`; use `-WdkRoot` if installed
elsewhere. ARM64 support is intentionally not included.

To install Qt somewhere else or use a later Qt patch release, pass `-QtRoot`
or `-QtVersion` to `prepare.ps1`, then pass the resulting
`<QtRoot>\<QtVersion>\msvc2022_64` directory to `build.ps1` with `-QtRoot`.
Likewise, pass `-VcpkgRoot` to both scripts when vcpkg is not installed under
`%USERPROFILE%\vcpkg`.
`prepare.ps1` checks out vcpkg at the commit pinned by `vcpkg.json`; override
that commit only when updating the manifest baseline with `-VcpkgCommit`.

`prepare.ps1` clones the pinned libwdi v1.5.1 source release beside qcma (for
example, `Z:\libwdi` when qcma is in `Z:\qcma`) if it is absent. It is not a
submodule. To use an existing checkout elsewhere, pass its path to both
scripts with `-LibWdiSource`.

`qcma-winusb.exe` is a small, fixed-purpose PS Vita helper. It uses libwdi to
prepare and sign the driver. The installer includes libwdi's LGPL-3.0 license
and source location notice.

The installer offers an optional **Install PS Vita WinUSB driver** component.
It creates a device-specific driver package and a one-use local signing
certificate before installing it. If the Vita is connected, Windows applies the
driver immediately; otherwise it applies it on the next connection. This avoids
shipping external driver installers. If the component is declined or does not work, use
[Zadig](https://zadig.akeo.ie/): select the PS Vita device, choose **WinUSB**,
and install the driver.
