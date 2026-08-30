Unicode true

!include "MUI2.nsh"
!include "x64.nsh"

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
InstallDirRegKey HKLM "${UNINSTALL_KEY}" "InstallLocation"
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

Section "Qcma (required)" SecQcma
  SectionIn RO
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  File /r "${PAYLOAD_DIR}\*"

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

!ifdef HAVE_DRIVER_WINUSB
Section "WinUSB driver" SecWinUsb
  SetOutPath "$INSTDIR\driver"
  File "${DRIVER_DIR}\QcmaDriver_winusb.exe"
  ExecWait '$"$INSTDIR\driver\QcmaDriver_winusb.exe$"' $0
  IntCmp $0 0 +2
  MessageBox MB_ICONEXCLAMATION "The WinUSB driver installer returned exit code $0."
SectionEnd
!endif

!ifdef HAVE_DRIVER_LIBUSB0
Section /o "libusb0 driver" SecLibusb0
  SetOutPath "$INSTDIR\driver"
  File "${DRIVER_DIR}\QcmaDriver_libusb0.exe"
  ExecWait '$"$INSTDIR\driver\QcmaDriver_libusb0.exe$"' $0
  IntCmp $0 0 +2
  MessageBox MB_ICONEXCLAMATION "The libusb0 driver installer returned exit code $0."
SectionEnd
!endif

!ifdef HAVE_DRIVER_LIBUSBK
Section /o "libusbK driver" SecLibusbK
  SetOutPath "$INSTDIR\driver"
  File "${DRIVER_DIR}\QcmaDriver_libusbk.exe"
  ExecWait '$"$INSTDIR\driver\QcmaDriver_libusbk.exe$"' $0
  IntCmp $0 0 +2
  MessageBox MB_ICONEXCLAMATION "The libusbK driver installer returned exit code $0."
SectionEnd
!endif

Section "Uninstall"
  SetShellVarContext all
  SetRegView 64
  Delete "$SMPROGRAMS\Qcma\Qcma.lnk"
  Delete "$SMPROGRAMS\Qcma\Uninstall Qcma.lnk"
  RMDir "$SMPROGRAMS\Qcma"
  DeleteRegKey HKLM "${UNINSTALL_KEY}"
  RMDir /r "$INSTDIR"
SectionEnd
