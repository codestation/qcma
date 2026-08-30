$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Install-WingetPackage {
    param([Parameter(Mandatory = $true)][string]$Id)

    winget list --id $Id --exact --accept-source-agreements *> $null
    if ($LASTEXITCODE -ne 0) {
        winget install --id $Id --exact --accept-package-agreements --accept-source-agreements
        if ($LASTEXITCODE -ne 0) {
            throw "winget could not install $Id"
        }
    }
}

if (-not (Get-Command winget.exe -ErrorAction SilentlyContinue)) {
    throw 'winget is required. Install or update Microsoft App Installer first.'
}

Install-WingetPackage -Id 'MSYS2.MSYS2'
Install-WingetPackage -Id 'NSIS.NSIS'

$bash = 'C:\msys64\usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $bash)) {
    throw "MSYS2 was installed but $bash was not found"
}

$env:CHERE_INVOKING = '1'
$env:MSYSTEM = 'MSYS'

# Running the upgrade twice also completes updates where pacman itself was
# replaced during the first pass.
for ($pass = 1; $pass -le 2; $pass++) {
    & $bash -lc 'pacman -Syu --noconfirm'
    if ($LASTEXITCODE -ne 0) {
        throw "MSYS2 system update failed on pass $pass"
    }
}

$packages = @(
    'base-devel'
    'cmake'
    'git'
    'autoconf'
    'automake'
    'gettext-devel'
    'libtool'
    'make'
    'ninja'
    'pkgconf'
    'mingw-w64-ucrt-x86_64-toolchain'
    'mingw-w64-ucrt-x86_64-ffmpeg'
    'mingw-w64-ucrt-x86_64-libiconv'
    'mingw-w64-ucrt-x86_64-libusb'
    'mingw-w64-ucrt-x86_64-libxml2'
    'mingw-w64-ucrt-x86_64-ntldd'
    'mingw-w64-ucrt-x86_64-qt6-base'
    'mingw-w64-ucrt-x86_64-qt6-tools'
)

$packageList = $packages -join ' '
& $bash -lc "pacman -S --needed --noconfirm $packageList"
if ($LASTEXITCODE -ne 0) {
    throw 'Installing the MSYS2/UCRT64 build dependencies failed'
}

Write-Host 'Windows build environment is ready.'
Write-Host 'Run buildscripts\windows\build.ps1 from PowerShell to build Qcma.'
