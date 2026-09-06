Unicode true

!include "MUI2.nsh"

!ifndef PRODUCT_VERSION
  !error "PRODUCT_VERSION must be supplied by build.ps1"
!endif
!ifndef PAYLOAD_DIR
  !error "PAYLOAD_DIR must be supplied by build.ps1"
!endif
!ifndef OUTPUT_FILE
  !error "OUTPUT_FILE must be supplied by build.ps1"
!endif

!define PRODUCT_NAME "Qcma"
!define PRODUCT_EXE "qcma.exe"
!define PRODUCT_PUBLISHER "codestation"
!define PRODUCT_URL "https://github.com/codestation/qcma"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Qcma"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
SetRegView 64

VIProductVersion "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.0"
VIAddVersionKey /LANG=1033 "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=1033 "FileDescription" "Qcma installer"
VIAddVersionKey /LANG=1033 "LegalCopyright" "GPL-3.0-or-later"

!define MUI_ABORTWARNING
!define MUI_ICON "${PAYLOAD_DIR}\qcma.ico"
!define MUI_UNICON "${PAYLOAD_DIR}\qcma.ico"
!define MUI_FINISHPAGE_RUN "$INSTDIR\${PRODUCT_EXE}"
!define MUI_FINISHPAGE_RUN_NOTCHECKED

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${PAYLOAD_DIR}\COPYING.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Italian"

Function .onInit
  SetRegView 64
  ReadRegStr $0 HKLM "${UNINSTALL_KEY}" "InstallLocation"
  StrCmp $0 "" +2
  StrCpy $INSTDIR $0
FunctionEnd

Section "Qcma (required)" SecQcma
  SectionIn RO
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  File /r "${PAYLOAD_DIR}\*"
  FileOpen $0 "$INSTDIR\.qcma-install" w
  FileWrite $0 "${PRODUCT_NAME}$\r$\n"
  FileClose $0

  WriteUninstaller "$INSTDIR\uninstall.exe"
  CreateDirectory "$SMPROGRAMS\Qcma"
  CreateShortcut "$SMPROGRAMS\Qcma\Qcma.lnk" "$INSTDIR\${PRODUCT_EXE}"
  CreateShortcut "$SMPROGRAMS\Qcma\Uninstall Qcma.lnk" "$INSTDIR\uninstall.exe"

  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\qcma.ico"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "URLInfoAbout" "${PRODUCT_URL}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "UninstallString" '$"$INSTDIR\uninstall.exe$"'
  WriteRegStr HKLM "${UNINSTALL_KEY}" "QuietUninstallString" '$"$INSTDIR\uninstall.exe$" /S'
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair" 1
SectionEnd

Section /o "Install PS Vita WinUSB driver" SecWinUsb
  DetailPrint "Installing the PS Vita WinUSB driver..."
  ExecWait '$"$INSTDIR\driver\qcma-winusb.exe$"' $0
  IntCmp $0 0 +2
  MessageBox MB_ICONEXCLAMATION "The WinUSB driver installer returned exit code $0."
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecWinUsb} "Installs the PS Vita WinUSB driver. If the Vita is connected, Windows applies it immediately; otherwise it is applied when the Vita is connected. This creates a one-use local certificate so Windows can trust the driver package."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"
  SetShellVarContext all
  SetRegView 64
  IfFileExists "$INSTDIR\.qcma-install" marker_found
  MessageBox MB_ICONSTOP "Qcma's installation marker is missing. No files were removed."
  Abort
marker_found:
  Delete "$SMPROGRAMS\Qcma\Qcma.lnk"
  Delete "$SMPROGRAMS\Qcma\Uninstall Qcma.lnk"
  RMDir "$SMPROGRAMS\Qcma"
  DeleteRegKey HKLM "${UNINSTALL_KEY}"
  Delete "$INSTDIR\.qcma-install"
  SetOutPath "$TEMP"
  RMDir /r "$INSTDIR"
SectionEnd
