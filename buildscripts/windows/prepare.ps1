[CmdletBinding()]
param(
    [string]$VcpkgRoot = "$env:USERPROFILE\vcpkg",
    [string]$VcpkgCommit = '04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4',
    [string]$QtRoot = 'C:\Qt',
    [string]$QtVersion = '6.10.2',
    [string]$LibWdiSource
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Install-WingetPackage([string]$Id) {
    winget list --id $Id --exact --accept-source-agreements *> $null
    if ($LASTEXITCODE -ne 0) {
        winget install --id $Id --exact --accept-package-agreements --accept-source-agreements
        if ($LASTEXITCODE -ne 0) { throw "Could not install $Id" }
    }
}

function Test-IsAdministrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-VisualCppBuildTools([string]$VsWhere) {
    if (-not (Test-Path $VsWhere)) { return $false }
    $installation = & $VsWhere -latest -products * -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    return -not [string]::IsNullOrWhiteSpace([string]$installation)
}

function Install-VisualCppBuildTools {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $setup = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\setup.exe'
    $installationPath = if (Test-Path $vswhere) {
        & $vswhere -latest -products * -property installationPath | Select-Object -First 1
    }
    $hasCppBuildTools = Test-VisualCppBuildTools $vswhere
    if (-not $hasCppBuildTools) {
        if (-not (Test-IsAdministrator)) {
            throw 'Visual C++ Build Tools must be installed from an elevated PowerShell. Open PowerShell with "Run as administrator", then run prepare.ps1 again.'
        }
        if ($installationPath -and (Test-Path $setup)) {
            & $setup modify --installPath $installationPath --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive --norestart
        } else {
            winget install --id Microsoft.VisualStudio.2022.BuildTools --exact --force --accept-package-agreements --accept-source-agreements --override '--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended'
        }
        if ($LASTEXITCODE -notin @(0, 3010)) { throw 'Could not install the Visual C++ Build Tools workload.' }
        for ($attempt = 0; $attempt -lt 180 -and -not $hasCppBuildTools; $attempt++) {
            Start-Sleep -Seconds 10
            $hasCppBuildTools = Test-VisualCppBuildTools $vswhere
        }
        if (-not $hasCppBuildTools) { throw 'Visual C++ Build Tools did not complete installation within 30 minutes. Check the Visual Studio Installer logs, then run prepare.ps1 again.' }
    }
}

function Test-Wdk8Redistributable {
    $wdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\8.0'
    foreach ($required in @(
        (Join-Path $wdkRoot 'redist\wdf\x64\WdfCoInstaller01011.dll')
        (Join-Path $wdkRoot 'redist\wdf\x64\winusbcoinstaller2.dll')
    )) {
        if (-not (Test-Path $required)) { return $false }
    }
    return $true
}

function Install-Wdk8Redistributable {
    if (Test-Wdk8Redistributable) { return }
    if (-not (Test-IsAdministrator)) {
        throw 'The Redistributable WDK 8 Components must be installed from an elevated PowerShell. Open PowerShell with "Run as administrator", then run prepare.ps1 again.'
    }

    $installer = Join-Path ([IO.Path]::GetTempPath()) 'qcma-wdfcoinstaller.msi'
    try {
        Invoke-WebRequest -Uri 'https://go.microsoft.com/fwlink/p/?LinkID=253170' -OutFile $installer -UseBasicParsing
        $process = Start-Process msiexec.exe -ArgumentList "/i `"$installer`" /passive /norestart" -Wait -PassThru
        if ($process.ExitCode -notin @(0, 3010)) { throw "Could not install the Redistributable WDK 8 Components (msiexec exit code $($process.ExitCode))." }
    } finally {
        if (Test-Path $installer) { Remove-Item -Force $installer }
    }

    if (-not (Test-Wdk8Redistributable)) { throw 'The Redistributable WDK 8 Components did not install the required WinUSB files.' }
}

function Update-SessionPath {
    $paths = @(
        ($env:Path -split ';')
        ([Environment]::GetEnvironmentVariable('Path', 'Machine') -split ';')
        ([Environment]::GetEnvironmentVariable('Path', 'User') -split ';')
    ) | Where-Object { $_ } | Select-Object -Unique
    $env:Path = $paths -join ';'
}

if (-not (Get-Command winget.exe -ErrorAction SilentlyContinue)) { throw 'winget is required.' }
Install-VisualCppBuildTools
Install-Wdk8Redistributable
@('Kitware.CMake', 'Git.Git', 'NSIS.NSIS', 'Python.Python.3.13') |
    ForEach-Object { Install-WingetPackage $_ }
Update-SessionPath
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$qcma = (Resolve-Path (Join-Path $scriptDir '..\..')).Path
if (-not $LibWdiSource) { $LibWdiSource = Join-Path (Split-Path -Parent $qcma) 'libwdi' }
if (-not (Test-Path $LibWdiSource)) {
    git clone --branch v1.5.1 --depth 1 https://github.com/pbatard/libwdi.git $LibWdiSource
    if ($LASTEXITCODE -ne 0) { throw 'Could not clone libwdi v1.5.1.' }
} elseif (-not (Test-Path (Join-Path $LibWdiSource 'libwdi.sln'))) {
    throw "Existing libwdi directory is not a valid source checkout: $LibWdiSource"
}
$qtInstall = Join-Path $QtRoot "$QtVersion\msvc2022_64"
if (-not (Test-Path (Join-Path $qtInstall 'bin\windeployqt.exe'))) {
    $pythonLauncher = Get-Command py.exe -ErrorAction SilentlyContinue
    if (-not $pythonLauncher) { throw 'Python 3.13 was installed but py.exe was not found in PATH.' }
    $python = $pythonLauncher.Source
    & $python -m pip install --user --upgrade aqtinstall
    if ($LASTEXITCODE -ne 0) { throw 'Could not install aqtinstall.' }
    & $python -m aqt install-qt windows desktop $QtVersion win64_msvc2022_64 --outputdir $QtRoot
    if ($LASTEXITCODE -ne 0) { throw 'Qt installation with aqtinstall failed.' }
}
if (-not (Test-Path "$VcpkgRoot\.git")) { git clone --filter=blob:none https://github.com/microsoft/vcpkg.git $VcpkgRoot }
git -C $VcpkgRoot fetch --depth 1 origin $VcpkgCommit
if ($LASTEXITCODE -ne 0) { throw "Could not fetch vcpkg commit $VcpkgCommit" }
git -C $VcpkgRoot checkout --detach $VcpkgCommit
if ($LASTEXITCODE -ne 0) { throw "Could not check out vcpkg commit $VcpkgCommit" }
& "$VcpkgRoot\bootstrap-vcpkg.bat" -disableMetrics
if ($LASTEXITCODE -ne 0) { throw 'vcpkg bootstrap failed.' }
Write-Host "Vcpkg is ready at $VcpkgRoot"
Write-Host "Qt $QtVersion is ready at $qtInstall."
