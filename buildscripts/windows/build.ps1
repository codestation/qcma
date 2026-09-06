[CmdletBinding()]
param(
    [string]$VitaMtpSource,
    [string]$LibWdiSource,
    [string]$OutputDirectory,
    [Parameter(Mandatory = $true)][string]$QtRoot,
    [string]$VcpkgRoot = $env:VCPKG_ROOT
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$qcma = (Resolve-Path (Join-Path $scriptDir '..\..')).Path
if (-not $VitaMtpSource) { $VitaMtpSource = Join-Path (Split-Path -Parent $qcma) 'vitamtp' }
if (-not $LibWdiSource) { $LibWdiSource = Join-Path (Split-Path -Parent $qcma) 'libwdi' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $qcma 'artifacts\windows' }
if (-not $VcpkgRoot) { throw 'Set VCPKG_ROOT or pass -VcpkgRoot.' }
$toolchain = Join-Path $VcpkgRoot 'scripts\buildsystems\vcpkg.cmake'
$qtDir = Join-Path $QtRoot 'lib\cmake\Qt6'
$deploy = Join-Path $QtRoot 'bin\windeployqt.exe'
foreach ($required in @((Join-Path $VitaMtpSource 'CMakeLists.txt'), (Join-Path $LibWdiSource 'libwdi.sln'), $toolchain, $qtDir, $deploy)) {
    if (-not (Test-Path $required)) { throw "Required path not found: $required" }
}
$work = Join-Path $OutputDirectory '.work'
$prefix = Join-Path $work 'vitamtp-prefix'
$payload = Join-Path $work 'payload'
$vitaBuild = Join-Path $work 'vitamtp-build'
$qcmaBuild = Join-Path $work 'qcma-build'
New-Item -ItemType Directory -Force $OutputDirectory, $prefix, $payload | Out-Null

cmake -S $VitaMtpSource -B $vitaBuild -A x64 "-DCMAKE_TOOLCHAIN_FILE=$toolchain" "-DCMAKE_INSTALL_PREFIX=$prefix"
cmake --build $vitaBuild --config Release
ctest --test-dir $vitaBuild -C Release --output-on-failure
cmake --install $vitaBuild --config Release
cmake -S $qcma -B $qcmaBuild -A x64 "-DCMAKE_TOOLCHAIN_FILE=$toolchain" "-DCMAKE_PREFIX_PATH=$prefix" "-DQt6_DIR=$qtDir"
cmake --build $qcmaBuild --config Release
ctest --test-dir $qcmaBuild -C Release --output-on-failure

$programFilesX86 = [Environment]::GetFolderPath('ProgramFilesX86')
$vswhere = Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
$msbuild = if (Test-Path $vswhere) {
    (& $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1)
} else {
    (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
}
if (-not $msbuild) { throw 'MSBuild was not found. Install the Visual Studio 2022 Build Tools.' }
& $msbuild (Join-Path $LibWdiSource 'libwdi.sln') /t:wdi-simple /p:Configuration=Release /p:Platform=x64
if ($LASTEXITCODE -ne 0) { throw 'libwdi wdi-simple build failed.' }
$winUsbInstaller = Join-Path $LibWdiSource 'x64\Release\examples\wdi-simple.exe'
if (-not (Test-Path $winUsbInstaller)) { throw "libwdi build did not produce $winUsbInstaller" }

Copy-Item (Join-Path $qcmaBuild 'gui\Release\qcma.exe') $payload -Force
Copy-Item (Join-Path $prefix 'bin\vitamtp.dll') $payload -Force
New-Item -ItemType Directory -Force (Join-Path $payload 'driver'), (Join-Path $payload 'licenses') | Out-Null
Copy-Item $winUsbInstaller (Join-Path $payload 'driver\qcma-winusb.exe') -Force
Copy-Item (Join-Path $LibWdiSource 'COPYING') (Join-Path $payload 'licenses\libwdi-LGPL-3.0.txt') -Force
Copy-Item (Join-Path $scriptDir 'libwdi-source.txt') (Join-Path $payload 'licenses\libwdi-source.txt') -Force
& $deploy --release --no-translations (Join-Path $payload 'qcma.exe')
Get-ChildItem (Join-Path $qcmaBuild 'vcpkg_installed\x64-windows\bin\*.dll') | Copy-Item -Destination $payload -Force
$version = [regex]::Match((& (Join-Path $payload 'qcma.exe') --version), '\d+\.\d+\.\d+').Value
if ($LASTEXITCODE -ne 0 -or -not $version) { throw 'qcma.exe smoke test failed.' }
$parts = $version.Split('.')
$installer = Join-Path $OutputDirectory "Qcma-$version-windows-x86_64-setup.exe"
$args = @('/V3', "/DPRODUCT_VERSION=$version", "/DVERSION_MAJOR=$($parts[0])", "/DVERSION_MINOR=$($parts[1])", "/DVERSION_PATCH=$($parts[2])", "/DPAYLOAD_DIR=$payload", "/DOUTPUT_FILE=$installer")
& (Get-Command makensis.exe -ErrorAction Stop).Source @args (Join-Path $scriptDir 'qcma.nsi')
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $installer)) { throw 'NSIS failed.' }
Write-Host "Created $installer"
