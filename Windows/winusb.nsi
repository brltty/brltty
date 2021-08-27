!define VARIANT "winusb"

!macro BrlttyInstall
!macroend

!macro BrlttyShortcut
!macroend

!macro BrlttyTweak
!define sysSetupCopyOEMInf "setupapi::SetupCopyOEMInf(t, t, i, i, i, i, *i, t) i"
	System::Get '${sysSetupCopyOEMInf}'
	Pop $0
	StrCmp $0 'error' nodriver
	MessageBox MB_YESNO|MB_DEFBUTTON2 $(msg_inswinusb) IDNO nodriver
	System::Call '${sysSetupCopyOEMInf}?e ("$INSTDIR\bin\brltty-winusb.inf", "$INSTDIR\bin", 1, 0, 0, 0, 0, 0) .r0'
nodriver:
!macroend

!macro BrlttyUninstall
	!insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "$SYSDIR\WdfCoInstaller01009.dll"
	!insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "$SYSDIR\winusbcoinstaller2.dll"
!macroend

!include brltty.nsi
