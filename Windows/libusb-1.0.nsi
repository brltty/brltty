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

 !define MUI_WELCOMEPAGE_TITLE $(msg_WelcomePageTitle)

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
		!define DISTDIR "brltty-win-${VERSION}-libusb-1.0"
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
	!insertmacro MUI_PAGE_LICENSE "${DISTDIR}\LICENSE-LGPL.txt"
	!insertmacro MUI_PAGE_DIRECTORY

;--------------------------------

	;Start Menu Folder Page Configuration
	!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
	!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${PRODUCT}"
	!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

	!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

	!insertmacro MUI_PAGE_INSTFILES
	!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.txt"
	!insertmacro MUI_PAGE_FINISH

	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

!define UNINSTALLOG_LOCALIZE ; necessary for localization of messages from the uninstallation log file

;Remember the installer language
!define MUI_LANGDLL_REGISTRY_ROOT "HKCU"
!define MUI_LANGDLL_REGISTRY_KEY "Software\${PRODUCT}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

!insertmacro MUI_LANGUAGE "English" ; default language
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Japanese"
;!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Italian"
;!insertmacro MUI_LANGUAGE "Dutch"
;!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "Swedish"
;!insertmacro MUI_LANGUAGE "Norwegian"
;!insertmacro MUI_LANGUAGE "NorwegianNynorsk"
!insertmacro MUI_LANGUAGE "Finnish"
;!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Polish"
;!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Slovak"
!insertmacro MUI_LANGUAGE "Croatian"
;!insertmacro MUI_LANGUAGE "Bulgarian"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Thai"
;!insertmacro MUI_LANGUAGE "Romanian"
;!insertmacro MUI_LANGUAGE "Latvian"
;!insertmacro MUI_LANGUAGE "Macedonian"
;!insertmacro MUI_LANGUAGE "Estonian"
;!insertmacro MUI_LANGUAGE "Turkish"
;!insertmacro MUI_LANGUAGE "Lithuanian"
;!insertmacro MUI_LANGUAGE "Catalan"
;!insertmacro MUI_LANGUAGE "Slovenian"
;!insertmacro MUI_LANGUAGE "Serbian"
;!insertmacro MUI_LANGUAGE "SerbianLatin"
;!insertmacro MUI_LANGUAGE "Arabic"
;!insertmacro MUI_LANGUAGE "Farsi"
;!insertmacro MUI_LANGUAGE "Hebrew"
;!insertmacro MUI_LANGUAGE "Indonesian"
;!insertmacro MUI_LANGUAGE "Mongolian"
;!insertmacro MUI_LANGUAGE "Luxembourgish"
;!insertmacro MUI_LANGUAGE "Albanian"
;!insertmacro MUI_LANGUAGE "Breton"
;!insertmacro MUI_LANGUAGE "Belarusian"
;!insertmacro MUI_LANGUAGE "Icelandic"
;!insertmacro MUI_LANGUAGE "Malay"
;!insertmacro MUI_LANGUAGE "Bosnian"
;!insertmacro MUI_LANGUAGE "Kurdish"
;!insertmacro MUI_LANGUAGE "Irish"
;!insertmacro MUI_LANGUAGE "Uzbek"
!insertmacro MUI_LANGUAGE "Galician"
;--------------------------------

;----------------------------
; Language strings
!include "${DISTDIR}\nsistrings.txt"

;--------------------------------
;Installer Sections

Section "install"

	SetOutPath "$INSTDIR"
	SetShellVarContext all

	;Stop the BRLTTY service if it is running.
	ExecWait "$SYSDIR\net.exe stop brlapi"

	File /r /x brltty.conf /x nsistrings.txt "${DISTDIR}\*"
	# Install the config file separately so we can avoid overwriting it.
	SetOverwrite off
	File /oname=etc\brltty.conf "${DISTDIR}\etc\brltty.conf"
	SetOverwrite IfNewer

	!insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "${DISTDIR}\bin\brlapi-0.6.dll" "$SYSDIR\brlapi-0.6.dll" "$SYSDIR"
	!insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "${DISTDIR}\bin\libusb-1.0.dll" "$SYSDIR\libusb-1.0.dll" "$SYSDIR"
	!insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "${DISTDIR}\bin\msvcr90.dll" "$SYSDIR\msvcr90.dll" "$SYSDIR"
	!insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "${DISTDIR}\bin\libiconv-2.dll" "$SYSDIR\libiconv-2.dll" "$SYSDIR"

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
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_brlttycnf).lnk" "$INSTDIR\brlttycnf.exe" -r
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_brlttydebug).lnk" "$INSTDIR\run-debug.bat"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_inssrv).lnk" "$INSTDIR\install.bat"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_rmvsrv).lnk" "$INSTDIR\uninstall.bat"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_uninstall).lnk" "$INSTDIR\Uninstall.exe"
		;CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_inslibusb).lnk" "$SYSDIR\rundll32" "libusb0.dll,usb_install_driver_np_rundll $INSTDIR\bin\brltty-libusb.inf"
		;CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_inslibusb).lnk" "$SYSDIR\rundll32" "libusb0.dll,usb_install_driver_np_rundll $INSTDIR\bin\brltty-libusb.inf"
		;CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_inslibusbfilter).lnk" "$SYSDIR\rundll32" "libusb0.dll,usb_install_service_np_rundll"
		;CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_uninslibusbfilter).lnk" "$SYSDIR\rundll32" "libusb0.dll,usb_uninstall_service_np_rundll"

	!insertmacro MUI_STARTMENU_WRITE_END

	;Remove the BRLTTY service if it exists.
	ExecWait "$INSTDIR\bin\brltty.exe -R"
	;Install usb inf.
	ExecWait "$INSTDIR\brlttycnf.exe"
	;MessageBox MB_YESNO|MB_DEFBUTTON2 $(msg_inslibusbfilter) IDNO nofilter
	;ExecWait "rundll32 libusb0.dll,usb_install_service_np_rundll"
;nofilter:
!define sysSetupCopyOEMInf "setupapi::SetupCopyOEMInf(t, t, i, i, i, i, *i, t) i"
	System::Get '${sysSetupCopyOEMInf}'
	Pop $0
	StrCmp $0 'error' nodriver
	MessageBox MB_YESNO|MB_DEFBUTTON2 $(msg_inslibusb) IDNO nodriver
	System::Call '${sysSetupCopyOEMInf}?e ("$INSTDIR\bin\brltty-libusb-1.0.inf", "$INSTDIR\bin", 1, 0, 0, 0, 0, 0) .r0'
nodriver:
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
	;ExecWait "rundll32 libusb0.dll,usb_uninstall_service_np_rundll"

	!insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "$SYSDIR\brlapi-0.6.dll"
	!insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "$SYSDIR\WdfCoInstaller01009.dll"
	!insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "$SYSDIR\winusbcoinstaller2.dll"

	RMDir /r "$INSTDIR"

	!insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder

	RMDir /r "$SMPROGRAMS\$StartMenuFolder"

	DeleteRegKey HKLM "Software\${PRODUCT}"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}"

SectionEnd
