# Windows x64 build

Qcma's supported Windows toolchain is Visual Studio 2022 (MSVC), vcpkg and
Qt 6 MSVC 2022 64-bit and libwdi. MinGW and MSYS2 are not used.

Run `prepare.ps1` from an elevated PowerShell to install/check Visual Studio
Build Tools, CMake, Git, NSIS 3.12 or later, the official Qt installer and
vcpkg. In the Qt installer select Qt 6 MSVC 2022 64-bit, then build with:

```powershell
$env:VCPKG_ROOT = "$env:USERPROFILE\vcpkg"
.\build.ps1 -QtRoot C:\Qt\<version>\msvc2022_64
```

The build expects an unmodified libwdi source checkout next to qcma, or an
explicit `-LibWdiSource` path. It is not a submodule. Clone the pinned source
release with:

```powershell
git clone --branch v1.5.1 --depth 1 https://github.com/pbatard/libwdi ..\libwdi
```

The bundled `qcma-winusb.exe` is libwdi's unmodified `examples/wdi-simple.c`.
The installer includes libwdi's LGPL-3.0 license and a source-location notice.

The script builds and tests VitaMTP and qcma, deploys Qt and vcpkg DLLs,
runs `qcma.exe --version`, builds libwdi's WinUSB installer helper, and creates
the NSIS installer in `artifacts\windows`.

The installer offers an optional **Install PS Vita WinUSB driver** component.
It creates a device-specific driver package and a one-use local signing
certificate before installing it. If the Vita is connected, Windows applies the
driver immediately; otherwise it applies it on the next connection. This avoids
shipping external driver installers. If the component is declined or does not work, use
[Zadig](https://zadig.akeo.ie/): select the PS Vita device, choose **WinUSB**,
and install the driver.
