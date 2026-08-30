[CmdletBinding()]
param(
    [string]$VitaMtpSource,
    [string]$OutputDirectory,
    [string]$DriverDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$qcmaSource = (Resolve-Path (Join-Path $scriptDirectory '..\..')).Path

if ([string]::IsNullOrWhiteSpace($VitaMtpSource)) {
    $VitaMtpSource = Join-Path (Split-Path -Parent $qcmaSource) 'vitamtp'
}
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $qcmaSource 'artifacts\windows'
}
if ([string]::IsNullOrWhiteSpace($DriverDirectory)) {
    $DriverDirectory = Join-Path $scriptDirectory 'driver'
}

$VitaMtpSource = (Resolve-Path -LiteralPath $VitaMtpSource).Path
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
$DriverDirectory = [System.IO.Path]::GetFullPath($DriverDirectory)

if (-not (Test-Path -LiteralPath (Join-Path $VitaMtpSource 'configure.ac'))) {
    throw "VitaMTP source tree not found at $VitaMtpSource"
}

$bash = 'C:\msys64\usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $bash)) {
    throw 'MSYS2 is missing. Run buildscripts\windows\prepare.ps1 first.'
}

function ConvertTo-MsysPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ($Path.Contains("'")) {
        throw "Paths containing apostrophes are not supported: $Path"
    }
    $converted = & $bash -lc 'cygpath -u "$1"' -- $Path
    if ($LASTEXITCODE -ne 0) {
        throw "Could not convert path for MSYS2: $Path"
    }
    return ($converted | Select-Object -Last 1).Trim()
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

$msysScript = ConvertTo-MsysPath (Join-Path $scriptDirectory 'build-msys2.sh')
$msysQcma = ConvertTo-MsysPath $qcmaSource
$msysVitaMtp = ConvertTo-MsysPath $VitaMtpSource
$msysOutput = ConvertTo-MsysPath $OutputDirectory
$env:CHERE_INVOKING = '1'
$env:MSYSTEM = 'UCRT64'
& $bash -lc 'exec /usr/bin/bash "$1" "$2" "$3" "$4"' -- $msysScript $msysQcma $msysVitaMtp $msysOutput
if ($LASTEXITCODE -ne 0) {
    throw 'The MSYS2/UCRT64 build failed'
}

$versionFile = Join-Path $OutputDirectory '.work\version.txt'
$payloadDirectory = Join-Path $OutputDirectory '.work\payload'
$version = (Get-Content -LiteralPath $versionFile -Raw).Trim()
$numericVersion = [regex]::Match($version, '\d+\.\d+\.\d+').Value
if ([string]::IsNullOrWhiteSpace($numericVersion)) {
    throw "Could not derive a numeric version from '$version'"
}
$parts = $numericVersion.Split('.')
$installer = Join-Path $OutputDirectory "Qcma-$version-windows-x86_64-setup.exe"

$makensisCandidates = @(
    (Get-Command makensis.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
    "$env:ProgramFiles\NSIS\makensis.exe",
    "${env:ProgramFiles(x86)}\NSIS\makensis.exe"
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }
$makensis = $makensisCandidates | Select-Object -First 1
if (-not $makensis) {
    throw 'makensis.exe was not found. Run prepare.ps1 or install NSIS.'
}

$arguments = @(
    '/V3',
    "/DPRODUCT_VERSION=$version",
    "/DVERSION_MAJOR=$($parts[0])",
    "/DVERSION_MINOR=$($parts[1])",
    "/DVERSION_PATCH=$($parts[2])",
    "/DPAYLOAD_DIR=$payloadDirectory",
    "/DOUTPUT_FILE=$installer",
    "/DDRIVER_DIR=$DriverDirectory"
)

$drivers = @{
    'QcmaDriver_winusb.exe' = 'HAVE_DRIVER_WINUSB'
    'QcmaDriver_libusb0.exe' = 'HAVE_DRIVER_LIBUSB0'
    'QcmaDriver_libusbk.exe' = 'HAVE_DRIVER_LIBUSBK'
}
foreach ($entry in $drivers.GetEnumerator()) {
    if (Test-Path -LiteralPath (Join-Path $DriverDirectory $entry.Key)) {
        $arguments += "/D$($entry.Value)"
    }
}

Remove-Item -LiteralPath $installer -Force -ErrorAction SilentlyContinue
& $makensis @arguments (Join-Path $scriptDirectory 'qcma.nsi')
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $installer)) {
    throw 'NSIS did not produce the expected installer'
}

Write-Host "Created $installer"
if (-not ($drivers.Keys | Where-Object { Test-Path -LiteralPath (Join-Path $DriverDirectory $_) })) {
    Write-Warning 'No external USB driver installers were supplied; the Qcma installer contains the application only.'
}
