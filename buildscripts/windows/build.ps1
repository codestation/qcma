[CmdletBinding()]
param(
    [string]$VitaMtpSource,
    [string]$LibWdiSource,
    [string]$OutputDirectory,
    [string]$QtRoot = 'C:\Qt\6.10.2\msvc2022_64',
    [string]$VcpkgRoot = "$env:USERPROFILE\vcpkg",
    [string]$VcpkgTriplet = 'x64-windows-release',
    [string]$WdkRoot = 'C:\Program Files (x86)\Windows Kits\8.0'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$qcma = (Resolve-Path (Join-Path $scriptDir '..\..')).Path
if (-not $VitaMtpSource) { $VitaMtpSource = Join-Path (Split-Path -Parent $qcma) 'vitamtp' }
if (-not $LibWdiSource) { $LibWdiSource = Join-Path (Split-Path -Parent $qcma) 'libwdi' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $qcma 'artifacts\windows' }
if (-not $VcpkgRoot) { throw 'Set VCPKG_ROOT or pass -VcpkgRoot.' }
$libWdiSolution = Join-Path $LibWdiSource 'libwdi.sln'
$toolchain = Join-Path $VcpkgRoot 'scripts\buildsystems\vcpkg.cmake'
$tripletDirectory = Join-Path $scriptDir 'triplets'
$generator = 'Visual Studio 17 2022'
$qtDir = Join-Path $QtRoot 'lib\cmake\Qt6'
$deploy = Join-Path $QtRoot 'bin\windeployqt.exe'
foreach ($required in @((Join-Path $VitaMtpSource 'CMakeLists.txt'), $libWdiSolution, $toolchain, $qtDir, $deploy)) {
    if (-not (Test-Path $required)) { throw "Required path not found: $required" }
}
$work = Join-Path $OutputDirectory '.work'
$prefix = Join-Path $work 'vitamtp-prefix'
$payload = Join-Path $work 'payload'
$vitaBuild = Join-Path $work 'vitamtp-build'
$qcmaBuild = Join-Path $work 'qcma-build'
New-Item -ItemType Directory -Force $OutputDirectory, $prefix, $payload | Out-Null

cmake -S $VitaMtpSource -B $vitaBuild -G $generator -A x64 "-DCMAKE_TOOLCHAIN_FILE=$toolchain" "-DCMAKE_INSTALL_PREFIX=$prefix" "-DVCPKG_TARGET_TRIPLET=$VcpkgTriplet" "-DVCPKG_OVERLAY_TRIPLETS=$tripletDirectory"
cmake --build $vitaBuild --config Release
ctest --test-dir $vitaBuild -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'VitaMTP tests failed.' }
cmake --install $vitaBuild --config Release
cmake -S $qcma -B $qcmaBuild -G $generator -A x64 "-DCMAKE_TOOLCHAIN_FILE=$toolchain" "-DCMAKE_PREFIX_PATH=$prefix" "-DQt6_DIR=$qtDir" "-DVCPKG_TARGET_TRIPLET=$VcpkgTriplet" "-DVCPKG_OVERLAY_TRIPLETS=$tripletDirectory"
cmake --build $qcmaBuild --config Release
$originalPath = $env:Path
try {
    $env:Path = @(
        (Join-Path $QtRoot 'bin')
        (Join-Path $prefix 'bin')
        (Join-Path $qcmaBuild "vcpkg_installed\$VcpkgTriplet\bin")
        $env:Path
    ) -join ';'
    ctest --test-dir $qcmaBuild -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'qcma tests failed.' }
} finally {
    $env:Path = $originalPath
}

$programFilesX86 = [Environment]::GetFolderPath('ProgramFilesX86')
$vswhere = Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
$msbuild = if (Test-Path $vswhere) {
    (& $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1)
} else {
    (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
}
if (-not $msbuild) { throw 'MSBuild was not found. Install the Visual Studio 2022 Build Tools.' }

# libwdi's solution always schedules installer_arm64.  qcma ships an x64
# installer, so build only its required projects from an isolated source copy.
$libWdiBuildSource = Join-Path $work 'libwdi-x64'
if (Test-Path $libWdiBuildSource) { Remove-Item -Recurse -Force $libWdiBuildSource }
New-Item -ItemType Directory -Force $libWdiBuildSource | Out-Null
Get-ChildItem -LiteralPath $LibWdiSource -Force |
    Where-Object { $_.Name -ne '.git' } |
    Copy-Item -Destination $libWdiBuildSource -Recurse -Force

$libWdiConfig = Join-Path $libWdiBuildSource 'msvc\config.h'
$config = [IO.File]::ReadAllText($libWdiConfig)
$wdkFiles = @(
    (Join-Path $WdkRoot 'redist\wdf\x64\WdfCoInstaller01011.dll')
    (Join-Path $WdkRoot 'redist\wdf\x64\winusbcoinstaller2.dll')
)
if ($wdkFiles.Where({ -not (Test-Path $_) }).Count -ne 0) {
    throw "libwdi requires the Redistributable WDK 8 Components for WinUSB. Install them, or pass -WdkRoot with their directory: $WdkRoot"
}
$wdkRootForC = $WdkRoot.Replace('\', '/')
$config = [regex]::Replace($config, '(?m)^#define WDK_DIR\b.*$', "#define WDK_DIR `"$wdkRootForC`"")
foreach ($macro in @('LIBUSB0_DIR', 'LIBUSBK_DIR', 'OPT_ARM')) {
    $pattern = "(?m)^#define $macro\b.*$"
    if (-not [regex]::IsMatch($config, $pattern)) { throw "Could not disable $macro in $libWdiConfig" }
    $config = [regex]::Replace($config, $pattern, "// $macro is intentionally excluded from qcma's x64 build.")
}
[IO.File]::WriteAllText($libWdiConfig, $config)

$libWdiStaticProject = Join-Path $libWdiBuildSource 'libwdi\.msvc\libwdi_static.vcxproj'
[xml]$libWdiStaticXml = Get-Content -Raw $libWdiStaticProject
$namespace = [Xml.XmlNamespaceManager]::new($libWdiStaticXml.NameTable)
$namespace.AddNamespace('msb', 'http://schemas.microsoft.com/developer/msbuild/2003')
foreach ($reference in @($libWdiStaticXml.SelectNodes('//msb:ProjectReference', $namespace))) {
    [void]$reference.ParentNode.RemoveChild($reference)
}
$libWdiStaticXml.Save($libWdiStaticProject)

# Replace libwdi's generic sample with qcma's fixed-purpose PS Vita helper.
Copy-Item (Join-Path $scriptDir 'qcma-winusb.c') (Join-Path $libWdiBuildSource 'examples\wdi-simple.c') -Force

function Invoke-LibWdiMsBuild([string]$Project, [string]$Platform, [switch]$SkipProjectReferences) {
    $arguments = @(
        $Project
        '/p:Configuration=Release'
        "/p:Platform=$Platform"
        "/p:SolutionDir=$libWdiBuildSource\"
    )
    if ($SkipProjectReferences) { $arguments += '/p:BuildProjectReferences=false' }
    & $msbuild @arguments
    if ($LASTEXITCODE -ne 0) { throw "libwdi build failed: $Project ($Platform)" }
}

Invoke-LibWdiMsBuild (Join-Path $libWdiBuildSource 'libwdi\.msvc\detect_64build.vcxproj') x64
Invoke-LibWdiMsBuild (Join-Path $libWdiBuildSource 'libwdi\.msvc\embedder.vcxproj') Win32 -SkipProjectReferences
Invoke-LibWdiMsBuild (Join-Path $libWdiBuildSource 'libwdi\.msvc\installer_x64.vcxproj') x64
Invoke-LibWdiMsBuild (Join-Path $libWdiBuildSource 'libwdi\.msvc\libwdi_static.vcxproj') x64 -SkipProjectReferences
Invoke-LibWdiMsBuild (Join-Path $libWdiBuildSource 'examples\getopt\.msvc\getopt.vcxproj') x64
Invoke-LibWdiMsBuild (Join-Path $libWdiBuildSource 'examples\.msvc\wdi-simple.vcxproj') x64 -SkipProjectReferences

$winUsbInstaller = Join-Path $libWdiBuildSource 'x64\Release\examples\wdi-simple.exe'
if (-not (Test-Path $winUsbInstaller)) { throw "libwdi build did not produce $winUsbInstaller" }

Copy-Item (Join-Path $qcmaBuild 'gui\Release\qcma.exe') $payload -Force
Copy-Item (Join-Path $qcma 'gui\resources\images\qcma.ico') $payload -Force
Copy-Item (Join-Path $qcma 'COPYING') (Join-Path $payload 'COPYING.txt') -Force
Copy-Item (Join-Path $prefix 'bin\vitamtp.dll') $payload -Force
New-Item -ItemType Directory -Force (Join-Path $payload 'driver'), (Join-Path $payload 'licenses') | Out-Null
Copy-Item $winUsbInstaller (Join-Path $payload 'driver\qcma-winusb.exe') -Force
Copy-Item (Join-Path $LibWdiSource 'COPYING') (Join-Path $payload 'licenses\libwdi-LGPL-3.0.txt') -Force
Copy-Item (Join-Path $scriptDir 'libwdi-source.txt') (Join-Path $payload 'licenses\libwdi-source.txt') -Force
& $deploy --release --no-translations (Join-Path $payload 'qcma.exe')
if ($LASTEXITCODE -ne 0) { throw 'windeployqt failed.' }
$sqliteDriver = Join-Path $QtRoot 'plugins\sqldrivers\qsqlite.dll'
if (-not (Test-Path $sqliteDriver)) { throw "Qt SQLite driver was not found: $sqliteDriver" }
New-Item -ItemType Directory -Force (Join-Path $payload 'sqldrivers') | Out-Null
Copy-Item $sqliteDriver (Join-Path $payload 'sqldrivers\qsqlite.dll') -Force
Get-ChildItem (Join-Path $qcmaBuild "vcpkg_installed\$VcpkgTriplet\bin\*.dll") | Copy-Item -Destination $payload -Force
foreach ($required in @(
    'qcma.exe'
    'qcma.ico'
    'COPYING.txt'
    'vitamtp.dll'
    'driver\qcma-winusb.exe'
    'licenses\libwdi-LGPL-3.0.txt'
    'licenses\libwdi-source.txt'
    'platforms\qwindows.dll'
    'sqldrivers\qsqlite.dll'
)) {
    if (-not (Test-Path (Join-Path $payload $required))) { throw "Required installer payload file was not deployed: $required" }
}
$originalPath = $env:Path
Push-Location $payload
try {
    $env:Path = "$payload;$env:Path"
    & '.\qcma.exe' --version
    $versionExitCode = $LASTEXITCODE
} finally {
    Pop-Location
    $env:Path = $originalPath
}
$version = [regex]::Match((Get-Content -Raw (Join-Path $qcma 'CMakeLists.txt')), '(?m)^project\(qcma VERSION ([0-9]+\.[0-9]+\.[0-9]+)').Groups[1].Value
if ($versionExitCode -ne 0 -or -not $version) { throw 'qcma.exe smoke test failed or the project version could not be read.' }
$parts = $version.Split('.')
$installer = Join-Path $OutputDirectory "Qcma-$version-windows-x86_64-setup.exe"
$args = @('/V3', "/DPRODUCT_VERSION=$version", "/DVERSION_MAJOR=$($parts[0])", "/DVERSION_MINOR=$($parts[1])", "/DVERSION_PATCH=$($parts[2])", "/DPAYLOAD_DIR=$payload", "/DOUTPUT_FILE=$installer")
$makensisCommand = Get-Command makensis.exe -ErrorAction SilentlyContinue
$makensis = if ($makensisCommand) { $makensisCommand.Source }
if (-not $makensis) {
    $nsisLocations = @(
        (Join-Path ${env:ProgramFiles(x86)} 'NSIS\makensis.exe')
        (Join-Path $env:ProgramFiles 'NSIS\makensis.exe')
    )
    $makensis = $nsisLocations | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $makensis) { throw 'makensis.exe was not found. Run prepare.ps1 to install NSIS.' }
& $makensis @args (Join-Path $scriptDir 'qcma.nsi')
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $installer)) { throw 'NSIS failed.' }
Write-Host "Created $installer"
