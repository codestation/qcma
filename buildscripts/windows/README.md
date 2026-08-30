# Windows x86_64

The native Windows build uses the current MSYS2 UCRT64 packages, Qt 6 and
NSIS. It supports Windows 10 1809 or newer and builds VitaMTP before Qcma.
Run these commands in PowerShell on a Windows x86_64 host:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\buildscripts\windows\prepare.ps1
.\buildscripts\windows\build.ps1
```

`prepare.ps1` installs or updates MSYS2 and NSIS through `winget`, then
installs the UCRT64 toolchain and dependencies through `pacman`. It is safe to
run again when refreshing a build host.

`build.ps1` expects VitaMTP in a sibling `vitamtp` checkout and writes the
installer to `artifacts\windows`. Both locations can be overridden:

```powershell
.\buildscripts\windows\build.ps1 `
  -VitaMtpSource C:\src\vitamtp `
  -OutputDirectory C:\artifacts `
  -DisableFfmpeg
```

Qt dependencies are deployed with `windeployqt`; `ntldd` then resolves and
validates the remaining UCRT64 dependency closure. Only the GUI executable is
packaged because Qcma's headless target is Linux-only.

## Optional USB drivers

The repository does not distribute generated driver installers. If any of the
following files are present in `buildscripts\windows\driver`, or in a directory
passed through `-DriverDirectory`, their corresponding NSIS sections are
included automatically:

- `QcmaDriver_winusb.exe`
- `QcmaDriver_libusb0.exe`
- `QcmaDriver_libusbk.exe`

Without them the application installer is still produced. Install a suitable
WinUSB driver separately (for example with Zadig) before using USB transport.

The installer is not Authenticode-signed, so Windows may display a SmartScreen
warning. Native installation, USB and Wi-Fi tests must be completed on a real
Windows host before publishing it.
