# Windows x64 build

Qcma's supported Windows toolchain is Visual Studio 2022 (MSVC), vcpkg and
Qt 6 MSVC 2022 64-bit. MinGW and MSYS2 are not used.

Run `prepare.ps1` from an elevated PowerShell to install/check Visual Studio
Build Tools, CMake, Git, NSIS, the official Qt installer and vcpkg. In the Qt
installer select Qt 6 MSVC 2022 64-bit, then build with:

```powershell
$env:VCPKG_ROOT = "$env:USERPROFILE\vcpkg"
.\build.ps1 -QtRoot C:\Qt\6.8.3\msvc2022_64
```

The script builds and tests VitaMTP and qcma, deploys Qt and vcpkg DLLs,
runs `qcma.exe --version`, and creates the NSIS installer in
`artifacts\windows`.
