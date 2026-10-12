; TinyShell installer (NSIS 3, Unicode). Built by build/build-installer.sh:
;   makensis -DVERSION=0.8.0 -DEXE=tinyshell.exe -DOUTFILE=TinyShell-Setup.exe tinyshell.nsi
; Per-user install, no administrator rights: %LOCALAPPDATA%\TinyShell (where TinyShell also
; keeps config.ini and reports), Start menu shortcut, optional desktop shortcut, the user
; PATH, and an entry in Programs and Features with an uninstaller. English or Spanish,
; following Windows' display language.

Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WinMessages.nsh"
!include "StrFunc.nsh"
${StrStr}
${UnStrStr}
${UnStrRep}

!ifndef VERSION
  !error "pass -DVERSION=X.Y.Z"
!endif
!ifndef EXE
  !error "pass -DEXE=path/to/tinyshell.exe"
!endif
!ifndef OUTFILE
  !define OUTFILE "TinyShell-Setup.exe"
!endif

!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\TinyShell"
!define SITE "https://tinyshell.lucasan-videla.workers.dev"
; The user PATH is read and written whole; NSIS strings are 1024 characters, so a longer
; PATH is left untouched rather than risk truncating it (the Start menu shortcut still works).
!define PATH_LIMIT 900

Name "TinyShell"
OutFile "${OUTFILE}"
InstallDir "$LOCALAPPDATA\TinyShell"
RequestExecutionLevel user
SetCompressor /SOLID lzma
BrandingText "TinyShell ${VERSION}"
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName" "TinyShell"
VIAddVersionKey /LANG=1033 "FileDescription" "TinyShell ${VERSION} setup"
VIAddVersionKey /LANG=1033 "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "CompanyName" "TinyShell (Lucas Videla)"
VIAddVersionKey /LANG=1033 "LegalCopyright" "MIT License, Lucas Videla"

!define MUI_ICON "..\..\assets\tinyshell.ico"
!define MUI_UNICON "..\..\assets\tinyshell.ico"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\tinyshell.exe"
!define MUI_FINISHPAGE_RUN_PARAMETERS "login"
!define MUI_FINISHPAGE_RUN_TEXT "$(RunLogin)"
!define MUI_FINISHPAGE_SHOWREADME ""
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME_TEXT "$(DesktopShortcut)"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION DesktopShortcut
!define MUI_FINISHPAGE_LINK "${SITE}"
!define MUI_FINISHPAGE_LINK_LOCATION "${SITE}"
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Spanish"

LangString RunLogin ${LANG_ENGLISH} "Set it up now (tinyshell login)"
LangString RunLogin ${LANG_SPANISH} "Configurarlo ahora (tinyshell login)"
LangString DesktopShortcut ${LANG_ENGLISH} "Create a desktop shortcut"
LangString DesktopShortcut ${LANG_SPANISH} "Crear un acceso directo en el escritorio"
LangString ShortcutDesc ${LANG_ENGLISH} "Terminal agent for old PCs"
LangString ShortcutDesc ${LANG_SPANISH} "Agente de terminal para PCs viejas"
LangString UninstallLink ${LANG_ENGLISH} "Uninstall TinyShell"
LangString UninstallLink ${LANG_SPANISH} "Desinstalar TinyShell"
LangString PathAdded ${LANG_ENGLISH} "Added to PATH: you can type tinyshell in a new CMD window."
LangString PathAdded ${LANG_SPANISH} "Agregado al PATH: podés escribir tinyshell en una ventana nueva de CMD."
LangString PathTooLong ${LANG_ENGLISH} "Your PATH is long; it was left unchanged. Use the Start menu shortcut."
LangString PathTooLong ${LANG_SPANISH} "Tu PATH es largo; no se modificó. Usá el acceso del menú Inicio."
LangString RemoveData ${LANG_ENGLISH} "Also delete your settings, saved reports and the saved API key?"
LangString RemoveData ${LANG_SPANISH} "¿Borrar también la configuración, los reportes guardados y la API key guardada?"

Function DesktopShortcut
  SetOutPath "$PROFILE"
  CreateShortcut "$DESKTOP\TinyShell.lnk" "$INSTDIR\tinyshell.exe" "" "$INSTDIR\tinyshell.exe" 0 SW_SHOWNORMAL "" "$(ShortcutDesc)"
FunctionEnd

; Appends $INSTDIR to the user PATH unless it is already there or the PATH is too long.
Function AddToPath
  ReadRegStr $0 HKCU "Environment" "Path"
  StrLen $1 $0
  ${If} $1 > ${PATH_LIMIT}
    DetailPrint "$(PathTooLong)"
    Return
  ${EndIf}
  ${StrStr} $2 ";$0;" ";$INSTDIR;"
  ${If} $2 != ""
    Return
  ${EndIf}
  ${If} $0 == ""
    WriteRegExpandStr HKCU "Environment" "Path" "$INSTDIR"
  ${Else}
    WriteRegExpandStr HKCU "Environment" "Path" "$0;$INSTDIR"
  ${EndIf}
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
  DetailPrint "$(PathAdded)"
FunctionEnd

Section "TinyShell"
  SetOutPath "$INSTDIR"
  File "/oname=tinyshell.exe" "${EXE}"
  WriteUninstaller "$INSTDIR\uninstall.exe"

  SetOutPath "$PROFILE" ; the shortcut starts in the user's folder
  CreateDirectory "$SMPROGRAMS\TinyShell"
  CreateShortcut "$SMPROGRAMS\TinyShell\TinyShell.lnk" "$INSTDIR\tinyshell.exe" "" "$INSTDIR\tinyshell.exe" 0 SW_SHOWNORMAL "" "$(ShortcutDesc)"
  CreateShortcut "$SMPROGRAMS\TinyShell\$(UninstallLink).lnk" "$INSTDIR\uninstall.exe"

  WriteRegStr HKCU "${UNINST_KEY}" "DisplayName" "TinyShell"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "${UNINST_KEY}" "Publisher" "TinyShell (Lucas Videla)"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayIcon" "$INSTDIR\tinyshell.exe"
  WriteRegStr HKCU "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKCU "${UNINST_KEY}" "URLInfoAbout" "${SITE}"
  WriteRegDWORD HKCU "${UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINST_KEY}" "NoRepair" 1
  WriteRegDWORD HKCU "${UNINST_KEY}" "EstimatedSize" 300

  Call AddToPath
SectionEnd

Section "Uninstall"
  ; Ask first: the saved key is removed with the program's own logout, before it is deleted.
  MessageBox MB_YESNO|MB_ICONQUESTION "$(RemoveData)" /SD IDNO IDNO keep_data
    ExecWait '"$INSTDIR\tinyshell.exe" logout'
    Delete "$INSTDIR\config.ini"
    Delete "$INSTDIR\telemetry.log"
    RMDir /r "$INSTDIR\reports"
  keep_data:

  Delete "$INSTDIR\tinyshell.exe"
  Delete "$INSTDIR\tinyshell.old.exe"
  Delete "$INSTDIR\tinyshell.new.exe"
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR" ; only if nothing is left
  RMDir /r "$SMPROGRAMS\TinyShell"
  Delete "$DESKTOP\TinyShell.lnk"
  DeleteRegKey HKCU "${UNINST_KEY}"

  ReadRegStr $0 HKCU "Environment" "Path"
  StrLen $1 $0
  ${If} $1 <= ${PATH_LIMIT}
    ${UnStrStr} $2 ";$0;" ";$INSTDIR;"
    ${If} $2 != ""
      ${UnStrRep} $3 ";$0;" ";$INSTDIR;" ";"
      StrCpy $3 $3 "" 1   ; drop the leading ";"
      StrCpy $3 $3 -1     ; and the trailing one
      WriteRegExpandStr HKCU "Environment" "Path" "$3"
      SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
    ${EndIf}
  ${EndIf}
SectionEnd
