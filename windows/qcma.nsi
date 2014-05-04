#### QCMA NSIS Install Script

### Includes
!include LogicLib.nsh
!include MUI.nsh
!include x64.nsh

### General information
!define PRODUCT_NAME "Qcma"
!define PRODUCT_EXE_NAME "qcma.exe"
!define PRODUCT_VERSION_MAJOR 0
!define PRODUCT_VERSION_MINOR 3
!define PRODUCT_VERSION_BUILD 2
!define PRODUCT_PUBLISHER "codestation"
!define PRODUCT_WEB_SITE "https://github.com/codestation/qcma"
!define HELPURL "https://github.com/xiannox/qcma/wiki"

### Macros
!macro VerifyUserIsAdmin
UserInfo::GetAccountType
pop $0
${If} $0 != "admin" ;Require admin rights on NT4+
    MessageBox mb_iconstop "Administrator rights required!"
    SetErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
    Quit
${EndIf}
!macroend

### Functions
Function finishPageRunFunction
    ExecShell "" "$INSTDIR\${PRODUCT_EXE_NAME}"
FunctionEnd

### Variables
Var StartMenuFolder

### Installer settings

# Set compression
SetCompressor /FINAL /SOLID lzma

# Require admin rights on NT6+ (When UAC is turned on)
RequestExecutionLevel admin

# This will be in the installer/uninstaller title bar
Name "${PRODUCT_NAME}"
OutFile "${PRODUCT_NAME}_setup_${PRODUCT_VERSION_MAJOR}.${PRODUCT_VERSION_MINOR}.${PRODUCT_VERSION_BUILD}.exe"
InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"

!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_ABORTWARNING

!define MUI_ICON "qcma.ico"
!define MUI_UNICON "qcma.ico"

## Add installer pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "COPYING.rtf"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN ""
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_RUN_FUNCTION finishPageRunFunction
!insertmacro MUI_PAGE_FINISH

## Add uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

## Define languages
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Japanese"

### Installer

function .onInit

    setShellVarContext all
    !insertmacro VerifyUserIsAdmin
    !insertmacro MUI_LANGDLL_DISPLAY

    ${If} ${RunningX64}
        StrCpy $InstDir "$ProgramFiles64\${PRODUCT_NAME}"
        SetRegView 64
    ${Else}
        StrCpy $InstDir "$ProgramFiles32\${PRODUCT_NAME}"
    ${EndIf}
functionEnd
 
section "install"
    # Files for the install directory - to build the installer, these should be in the same directory as the install script (this file)
    SetOutPath $InstDir

    # Files added here should be removed by the uninstaller (see section "uninstall")
    
    ${If} ${RunningX64}
        File "win_x86_64\qcma.exe"
        File "win_x86_64\qcma_console.exe"
        File "win_x86_64\avcodec-55.dll"
        File "win_x86_64\avformat-55.dll"
        File "win_x86_64\avutil-52.dll"
        File "win_x86_64\iconv.dll"
        File "win_x86_64\libgcc_s_seh-1.dll"
        File "win_x86_64\libjpeg-62.dll"
        File "win_x86_64\libpcre16-0.dll"
        File "win_x86_64\libpng16-16.dll"
        File "win_x86_64\libsqlite3-0.dll"
        File "win_x86_64\libstdc++-6.dll"
        File "win_x86_64\libtiff-5.dll"
        File "win_x86_64\libusb-1.0.dll"
        File "win_x86_64\libvitamtp-3.dll"
        File "win_x86_64\libwinpthread-1.dll"
        File "win_x86_64\libxml2-2.dll"
        File "win_x86_64\Qt5Core.dll"
        File "win_x86_64\Qt5Gui.dll"
        File "win_x86_64\Qt5Network.dll"
        File "win_x86_64\Qt5Sql.dll"
        File "win_x86_64\Qt5Widgets.dll"
        File "win_x86_64\swscale-2.dll"
        File "win_x86_64\zlib1.dll"
        
        SetOutPath "$INSTDIR\platforms"
        File "win_x86_64\platforms\qwindows.dll"
        
        SetOutPath "$INSTDIR\imageformats"
        File "win_x86_64\imageformats\qgif.dll"
        File "win_x86_64\imageformats\qjpeg.dll"
        File "win_x86_64\imageformats\qtiff.dll"

        SetOutPath "$INSTDIR\sqldrivers"
        File "win_x86_64\sqldrivers\qsqlite.dll"
    ${Else}
        File "win_i686\qcma.exe"
        File "win_i686\qcma_console.exe"
        File "win_i686\avcodec-55.dll"
        File "win_i686\avformat-55.dll"
        File "win_i686\avutil-52.dll"
        File "win_i686\iconv.dll"
        File "win_i686\libgcc_s_sjlj-1.dll"
        File "win_i686\libjpeg-62.dll"
        File "win_i686\libpcre16-0.dll"
        File "win_i686\libpng16-16.dll"
        File "win_i686\libsqlite3-0.dll"
        File "win_i686\libstdc++-6.dll"
        File "win_i686\libtiff-5.dll"
        File "win_i686\libusb-1.0.dll"
        File "win_i686\libvitamtp-3.dll"
        File "win_i686\libwinpthread-1.dll"
        File "win_i686\libxml2-2.dll"
        File "win_i686\Qt5Core.dll"
        File "win_i686\Qt5Gui.dll"
        File "win_i686\Qt5Sql.dll"
        File "win_i686\Qt5Network.dll"
        File "win_i686\Qt5Widgets.dll"
        File "win_i686\swscale-2.dll"
        File "win_i686\zlib1.dll"
        
        SetOutPath "$INSTDIR\platforms"
        File "win_i686\platforms\qwindows.dll"
        
        SetOutPath "$INSTDIR\imageformats"
        File "win_i686\imageformats\qgif.dll"
        File "win_i686\imageformats\qjpeg.dll"
        File "win_i686\imageformats\qtiff.dll"

        SetOutPath "$INSTDIR\sqldrivers"
        File "win_i686\sqldrivers\qsqlite.dll"

    ${EndIf}

    # Uninstaller - See function un.onInit and section "uninstall" for configuration
    WriteUninstaller "$InstDir\uninstall.exe"

    SetOutPath $InstDir

    # Start Menu
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
        CreateShortCut  "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_NAME}.lnk"                 "$INSTDIR\${PRODUCT_EXE_NAME}"
        CreateShortCut  "$SMPROGRAMS\$StartMenuFolder\Uninstall ${PRODUCT_NAME}.lnk"     "$INSTDIR\uninstall.exe"
    !insertmacro MUI_STARTMENU_WRITE_END
 
    # Registry information for add/remove programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayName"          "${PRODUCT_NAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "UninstallString"      "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "InstallLocation"      "$\"$INSTDIR$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayIcon"          "$\"$INSTDIR\qcma.ico$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "Publisher"            "${PRODUCT_PUBLISHER}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "HelpLink"             "$\"${HELPURL}$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "URLInfoAbout"         "$\"${PRODUCT_WEB_SITE}$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayVersion"       "${PRODUCT_VERSION_MAJOR}.${PRODUCT_VERSION_MINOR}.${PRODUCT_VERSION_BUILD}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "VersionMajor"       ${PRODUCT_VERSION_MAJOR}
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "VersionMinor"       ${PRODUCT_VERSION_MINOR}
    # There is no option for modifying or repairing the install
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "NoRepair" 1
sectionEnd

section "driver"
    SetOutPath $InstDir\driver
    File "QcmaDriver.exe"
    DetailPrint "Starting the driver installation"     
    ExecWait "$InstDir\driver\QcmaDriver.exe"   
sectionEnd
 
### Uninstaller
 
function un.onInit
    SetShellVarContext all
    !insertmacro VerifyUserIsAdmin
    !insertmacro MUI_LANGDLL_DISPLAY
functionEnd
 
section "uninstall"

    # Remove Start Menu launcher
    !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
    Delete "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall ${PRODUCT_NAME}.lnk"
    RMDir "$SMPROGRAMS\$StartMenuFolder"
    
    # Remove Desktop launcher
    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
 
    # Recursively remove the contents of $INSTDIR, then $INSTDIR itself
    RMDir /r "$INSTDIR"
    RMDir "$INSTDIR"
 
    # Remove uninstaller information from the registry
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
sectionEnd
