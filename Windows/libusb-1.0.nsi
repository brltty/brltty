!define VARIANT "libusb-1.0"

!macro BrlttyInstall
	!insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "${DISTDIR}\bin\libusb-1.0.dll" "$SYSDIR\libusb-1.0.dll" "$SYSDIR"
!macroend

!macro BrlttyShortcut
!macroend

!macro BrlttyTweak
!define sysSetupCopyOEMInf "setupapi::SetupCopyOEMInf(t, t, i, i, i, i, *i, t) i"
	System::Get '${sysSetupCopyOEMInf}'
	Pop $0
	StrCmp $0 'error' nodriver
	MessageBox MB_YESNO|MB_DEFBUTTON2 $(msg_inslibusb) IDNO nodriver
	System::Call '${sysSetupCopyOEMInf}?e ("$INSTDIR\bin\brltty-libusb-1.0.inf", "$INSTDIR\bin", 1, 0, 0, 0, 0, 0) .r0'
nodriver:
!macroend

!macro BrlttyUninstall
	!insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "$SYSDIR\WdfCoInstaller01009.dll"
	!insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "$SYSDIR\winusbcoinstaller2.dll"
!macroend

!include brltty.nsi
