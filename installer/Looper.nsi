Unicode true
!include "MUI2.nsh"
!include "x64.nsh"
!include "LogicLib.nsh"

!ifndef APP_EXE
  !error "Define APP_EXE as the path to the Release executable"
!endif
!ifndef WEBVIEW_BOOTSTRAPPER
  !error "Define WEBVIEW_BOOTSTRAPPER as the path to Microsoft's WebView2 bootstrapper"
!endif
!ifndef ASIO_LICENSE
  !error "Define ASIO_LICENSE as the path to the ASIO SDK license"
!endif
!ifndef JUCE_LICENSE
  !error "Define JUCE_LICENSE as the path to the JUCE license"
!endif
!ifndef OUTPUT_FILE
  !error "Define OUTPUT_FILE as the installer output path"
!endif

Name "The Looper Board Archetype"
OutFile "${OUTPUT_FILE}"
InstallDir "$LOCALAPPDATA\Programs\The Looper Board Archetype"
RequestExecutionLevel user
SetCompressor /SOLID lzma

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "This application requires 64-bit Windows."
    SetErrorLevel 1
    Abort
  ${EndIf}
FunctionEnd

Function CheckWebView
  SetRegView 32
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}" "pv"
  ${If} $0 == ""
  ${OrIf} $0 == "0.0.0.0"
    ReadRegStr $0 HKCU "Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}" "pv"
  ${EndIf}
FunctionEnd

Section "Install"
  Call CheckWebView
  ${If} $0 == ""
  ${OrIf} $0 == "0.0.0.0"
    InitPluginsDir
    SetOutPath "$PLUGINSDIR"
    File /oname=MicrosoftEdgeWebview2Setup.exe "${WEBVIEW_BOOTSTRAPPER}"
    DetailPrint "Installing Microsoft Edge WebView2 Runtime (internet connection required)..."
    ClearErrors
    ExecWait '"$PLUGINSDIR\MicrosoftEdgeWebview2Setup.exe" /silent /install' $1
    ${If} ${Errors}
    ${OrIf} $1 != 0
      MessageBox MB_ICONSTOP "WebView2 Runtime installation failed. Connect to the internet and try again."
      SetErrorLevel 1
      Abort
    ${EndIf}
    Call CheckWebView
    ${If} $0 == ""
    ${OrIf} $0 == "0.0.0.0"
      MessageBox MB_ICONSTOP "WebView2 Runtime was not detected after installation."
      SetErrorLevel 1
      Abort
    ${EndIf}
  ${EndIf}

  ClearErrors
  SetOutPath "$INSTDIR"
  File "/oname=The Looper Board Archetype.exe" "${APP_EXE}"
  File /oname=ASIO-LICENSE.txt "${ASIO_LICENSE}"
  File /oname=JUCE-LICENSE.md "${JUCE_LICENSE}"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateDirectory "$SMPROGRAMS\The Looper Board Archetype"
  CreateShortcut "$SMPROGRAMS\The Looper Board Archetype\The Looper Board Archetype.lnk" "$INSTDIR\The Looper Board Archetype.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\TheLooperBoardArchetype" "DisplayName" "The Looper Board Archetype"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\TheLooperBoardArchetype" "DisplayVersion" "0.1.0"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\TheLooperBoardArchetype" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\TheLooperBoardArchetype" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\TheLooperBoardArchetype" "NoRepair" 1
  ${If} ${Errors}
    MessageBox MB_ICONSTOP "Installation could not be completed. Close the app and check that the installation folder is writable."
    SetErrorLevel 1
    Abort
  ${EndIf}
SectionEnd

Section "Uninstall"
  ${If} ${FileExists} "$INSTDIR\The Looper Board Archetype.exe"
    ClearErrors
    Delete "$INSTDIR\The Looper Board Archetype.exe"
    ${If} ${Errors}
      MessageBox MB_ICONSTOP "Could not remove the application. Close it and try again."
      SetErrorLevel 1
      Abort
    ${EndIf}
  ${EndIf}
  Delete "$SMPROGRAMS\The Looper Board Archetype\The Looper Board Archetype.lnk"
  RMDir "$SMPROGRAMS\The Looper Board Archetype"
  Delete "$INSTDIR\ASIO-LICENSE.txt"
  Delete "$INSTDIR\JUCE-LICENSE.md"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\TheLooperBoardArchetype"
SectionEnd
