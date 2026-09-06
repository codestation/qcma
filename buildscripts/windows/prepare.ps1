[CmdletBinding()]
param([string]$VcpkgRoot = "$env:USERPROFILE\vcpkg")
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Install-WingetPackage([string]$Id) {
    winget list --id $Id --exact --accept-source-agreements *> $null
    if ($LASTEXITCODE -ne 0) {
        winget install --id $Id --exact --accept-package-agreements --accept-source-agreements
        if ($LASTEXITCODE -ne 0) { throw "Could not install $Id" }
    }
}

if (-not (Get-Command winget.exe -ErrorAction SilentlyContinue)) { throw 'winget is required.' }
@('Microsoft.VisualStudio.2022.BuildTools', 'Kitware.CMake', 'Git.Git', 'NSIS.NSIS', 'TheQtCompany.Qt') |
    ForEach-Object { Install-WingetPackage $_ }
if (-not (Test-Path "$VcpkgRoot\.git")) { git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot }
& "$VcpkgRoot\bootstrap-vcpkg.bat" -disableMetrics
if ($LASTEXITCODE -ne 0) { throw 'vcpkg bootstrap failed.' }
Write-Host "Vcpkg is ready at $VcpkgRoot"
Write-Host 'In the official Qt installer add Qt 6 MSVC 2022 64-bit, then pass its msvc2022_64 directory to build.ps1 as -QtRoot.'
