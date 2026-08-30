$ErrorActionPreference = "Stop"

function Get-InstalledApps
{
    if ([IntPtr]::Size -eq 4) {
        $regpath = 'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*'
    }
    else {
        $regpath = @(
            'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*'
            'HKLM:\Software\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*'
        )
    }
    Get-ItemProperty $regpath | .{process{if($_.DisplayName -and $_.UninstallString) { $_ } }} | Select DisplayName, Publisher, InstallDate, DisplayVersion, UninstallString |Sort DisplayName
}

# Install chocolatey
if (!(Get-Command choco -errorAction SilentlyContinue)) {
  echo "Installing chocolatey"
  iex ((new-object net.webclient).DownloadString('https://chocolatey.org/install.ps1'))
}

# Install Git
$result = Get-InstalledApps | where {$_.DisplayName -like "Git *"}
if($result -eq $null) {
  echo "Installing git"
  choco install -y git
}

# Install NSIS
$result = Get-InstalledApps | where {$_.DisplayName -like "Nullsoft Install System"}
if($result -eq $null) {
  echo "Installing nsis"
  choco install -y nsis
}

# Install OpenSSL
Set-Variable -Name "OpenSslVer" -Value "1.0.2j"

if (!(Test-Path "$PSScriptRoot\openssl-$OpenSslVer-i386-win32.zip")) {
  echo "Downloading openssl (32bits)"
  Invoke-WebRequest -Uri "https://indy.fulgan.com/SSL/openssl-$OpenSslVer-i386-win32.zip" -OutFile "$PSScriptRoot\openssl-$OpenSslVer-i386-win32.zip"
}

if (!(Test-Path "$PSScriptRoot\openssl-$OpenSslVer-x64_86-win64.zip")) {
  echo "Downloading openssl (64bits)"
  Invoke-WebRequest -Uri "https://indy.fulgan.com/SSL/openssl-$OpenSslVer-x64_86-win64.zip" -OutFile "$PSScriptRoot\openssl-$OpenSslVer-x64_86-win64.zip"
}

if (!(Test-Path "c:\openssl-$OpenSslVer-i386-win32")) {
  echo "Unpacking openssl (32bits)"
  Expand-Archive "$PSScriptRoot\openssl-$OpenSslVer-i386-win32.zip" -dest c:\openssl-$OpenSslVer-i386-win32
}

if (!(Test-Path "c:\openssl-$OpenSslVer-x64_86-win64")) {
  echo "Unpacking openssl (64bits)"
  Expand-Archive "$PSScriptRoot\openssl-$OpenSslVer-x64_86-win64.zip" -dest c:\openssl-$OpenSslVer-x64_86-win64
}

# Install Qt

if (!(Test-Path "$PSScriptRoot\qt-opensource-windows-x86-msvc2015-5.7.0.exe")) {
  echo "Downloading Qt (32bits)"
  Invoke-WebRequest -Uri "http://download.qt.io/official_releases/qt/5.7/5.7.0/qt-opensource-windows-x86-msvc2015-5.7.0.exe" -OutFile "$PSScriptRoot\qt-opensource-windows-x86-msvc2015-5.7.0.exe"
}

if (!(Test-Path "$PSScriptRoot\qt-opensource-windows-x86-msvc2015_64-5.7.0.exe")) {
  echo "Downloading Qt (64bits)"
  Invoke-WebRequest -Uri "http://download.qt.io/official_releases/qt/5.7/5.7.0/qt-opensource-windows-x86-msvc2015_64-5.7.0.exe" -OutFile "$PSScriptRoot\qt-opensource-windows-x86-msvc2015_64-5.7.0.exe"
}

if (!(Test-Path "c:\Qt\Qt5.7.0_32")) {
  if (!(Test-Path "c:\qt-opensource-windows-x86-msvc2015-5.7.0.exe")) {
    echo "Copying Qt installer to C: (32bits)"
    Copy-Item "$PSScriptRoot\qt-opensource-windows-x86-msvc2015-5.7.0.exe" "c:\qt-opensource-windows-x86-msvc2015-5.7.0.exe"
  }

  echo "Installing Qt (32bits)"
  Start-Process -FilePath "c:\qt-opensource-windows-x86-msvc2015-5.7.0.exe" -ArgumentList ("--script", "$PSScriptRoot\qt-noninteractive32.qs") -Wait
  Remove-Item "c:\qt-opensource-windows-x86-msvc2015-5.7.0.exe"
}

if (!(Test-Path "c:\Qt\Qt5.7.0_64")) {
  if (!(Test-Path "c:\qt-opensource-windows-x86-msvc2015_64-5.7.0.exe")) {
    echo "Copying Qt installer to C: (64bits)"
    Copy-Item "$PSScriptRoot\qt-opensource-windows-x86-msvc2015_64-5.7.0.exe" "c:\qt-opensource-windows-x86-msvc2015_64-5.7.0.exe"
  }

  echo "Installing Qt (64bits)"
  Start-Process -FilePath "c:\qt-opensource-windows-x86-msvc2015_64-5.7.0.exe" -ArgumentList ("--script", "$PSScriptRoot\qt-noninteractive64.qs") -Wait
  Remove-Item "c:\qt-opensource-windows-x86-msvc2015_64-5.7.0.exe"
}

# Install C++ Build tools
#$result = Get-InstalledApps | where {$_.DisplayName -like "Microsoft Visual C++ Build Tools"}
#if ($result -eq $null) {
#  echo "Installing VC BUild tools"
#  Start-Process -FilePath "$PSScriptRoot\vcbuildtools\VisualCppBuildTools_Full.exe" -ArgumentList ("/Passive", "/NoRestart", "/AdminFile", "$PSScriptRoot\AdminDeployment.xml") -Wait
#}

#$result = Get-InstalledApps | where {$_.DisplayName -like "Windows Software Development Kit - *"}
#if ($result -eq $null) {
#  echo "Installing SDK + Debugger"
#  Start-Process -FilePath "$PSScriptRoot\windowssdk/StandaloneSDK\sdksetup.exe" -ArgumentList ("/features", "OptionId.WindowsDesktopDebuggers", "/quiet") -Wait
#}

# Install MobaSSH
if (!(Test-Path "$PSScriptRoot\MobaSSH_Server_Home_1.60.zip")) {
  echo "Downloading MobaSSH"
  Invoke-WebRequest -Uri "http://mobassh.mobatek.net/MobaSSH_Server_Home_1.60.zip" -OutFile "$PSScriptRoot\MobaSSH_Server_Home_1.60.zip"
}

if (!(Test-Path "c:\MobaSSH_Server_Home_1.60.exe")) {
  echo "Unpacking MobaSSH"
  Expand-Archive "$PSScriptRoot\MobaSSH_Server_Home_1.60.zip" -dest c:\
}

# Install MSYS2
if (!(Test-Path "$PSScriptRoot\msys2-x86_64-20161025.exe")) {
  echo "Downloading MSYS2"
  Invoke-WebRequest -Uri "http://repo.msys2.org/distrib/x86_64/msys2-x86_64-20161025.exe" -OutFile "$PSScriptRoot\http://repo.msys2.org/distrib/x86_64/msys2-x86_64-20161025.exe"
}

if (!(Test-Path "c:\msys64")) {
  echo "Installing MSYS2"
  Start-Process -FilePath "$PSScriptRoot\msys2-x86_64-20161025.exe" -Wait
}
