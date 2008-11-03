;BRLTTY installer
;author: James Teh <jamie@jantrid.net>
;Based on the NSIS Modern User Interface Start Menu Folder Selection Example Script
;Written by Joost Verburg

;--------------------------------
;Include Modern UI

	!include "MUI2.nsh"
	!include "Library.nsh"

;--------------------------------
;Product Info

	!define PRODUCT "BRLTTY"
	!ifndef VERSION
		!error "VERSION is not defined"
	!endif

	!define MUI_WELCOMEPAGE_TITLE "Setup for ${PRODUCT}, Version ${VERSION}"

;--------------------------------
;General

	;Name and file
	Name "${PRODUCT}"
	Caption "${PRODUCT} ${VERSION} Setup"
	!ifdef OUTFILE
		OutFile "${OUTFILE}"
	!else
		OutFile "brltty-win-${VERSION}.exe"
	!endif

	!ifndef DISTDIR
		!define DISTDIR "brltty-win-${VERSION}"
	!endif

	SetCompressor /SOLID LZMA
	SetOverwrite IfNewer

	;Default installation folder
	InstallDir "$PROGRAMFILES\${PRODUCT}"

	;Get installation folder from registry if available
	InstallDirRegKey HKLM "Software\${PRODUCT}" ""

	;Request application privileges for Windows Vista
	RequestExecutionLevel admin

;--------------------------------
;Variables

	Var StartMenuFolder

;--------------------------------
;Interface Settings

	!define MUI_ABORTWARNING

;--------------------------------
;Pages

	!insertmacro MUI_PAGE_WELCOME
	!insertmacro MUI_PAGE_LICENSE "${DISTDIR}\copying.txt"
	!insertmacro MUI_PAGE_DIRECTORY

	;Start Menu Folder Page Configuration
	!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
	!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${PRODUCT}"
	!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

	!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

	!insertmacro MUI_PAGE_INSTFILES
	!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.install.txt"
	!insertmacro MUI_PAGE_FINISH
  
	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages
 
	!insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "install"

	SetOutPath "$INSTDIR"
	SetShellVarContext all

	;Stop the BRLTTY service if it is running.
	ExecWait "$SYSDIR\net.exe stop brlapi"

	File /r /x *install.bat /x brltty.conf "${DISTDIR}\*"
	# Install the config file separately so we can avoid overwriting it.
	SetOverwrite off
	File /oname=etc\brltty.conf "${DISTDIR}\etc\brltty.conf"
	SetOverwrite IfNewer

	!insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "${DISTDIR}\bin\brlapi-0.5.dll" "$SYSDIR\brlapi-0.5.dll" "$SYSDIR"

	;Store installation folder
	WriteRegStr HKLM "Software\${PRODUCT}" "" $INSTDIR

	;Create uninstaller
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayName" "${PRODUCT}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayVersion" "${VERSION}"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "NoModify" "1"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "NoRepair" "1"
  
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application

		;Create shortcuts
		CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

	!insertmacro MUI_STARTMENU_WRITE_END

	;Remove the BRLTTY service if it exists.
	ExecWait "$INSTDIR\bin\brltty.exe -R"
	;Install and start the BRLTTY service
	ExecWait "$INSTDIR\bin\brltty.exe -I"
	ExecWait "$SYSDIR\net.exe start brlapi"

SectionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

	SetShellVarContext all

	;Stop and remove the BRLTTY service
	ExecWait "$SYSDIR\net.exe stop brlapi"
	ExecWait "$INSTDIR\bin\brltty.exe -R"

	!insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "$SYSDIR\brlapi-0.5.dll"

	RMDir /r "$INSTDIR"

	!insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder

	RMDir /r "$SMPROGRAMS\$StartMenuFolder"

	DeleteRegKey HKLM "Software\${PRODUCT}"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}"

SectionEnd
