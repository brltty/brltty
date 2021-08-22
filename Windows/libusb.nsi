!define VARIANT "libusb"

!macro BrlttyInstall
	!insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "${DISTDIR}\bin\libusb0.dll" "$SYSDIR\libusb0.dll" "$SYSDIR"
	!insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "${DISTDIR}\bin\libusb0.sys" "$SYSDIR\drivers\libusb0.sys" "$SYSDIR\drivers"
!macroend

!macro BrlttyShortcut
	CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_inslibusb).lnk" "$SYSDIR\rundll32" "libusb0.dll,usb_install_driver_np_rundll $INSTDIR\bin\brltty-libusb.inf"
	CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_inslibusbfilter).lnk" "$SYSDIR\rundll32" "libusb0.dll,usb_install_service_np_rundll"
	CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(shortcut_uninslibusbfilter).lnk" "$SYSDIR\rundll32" "libusb0.dll,usb_uninstall_service_np_rundll"
!macroend

!macro BrlttyTweak
	MessageBox MB_YESNO|MB_DEFBUTTON2 $(msg_inslibusb32filter) IDNO nofilter
	ExecWait "rundll32 libusb0.dll,usb_install_service_np_rundll"
nofilter:
	MessageBox MB_YESNO|MB_DEFBUTTON2 $(msg_inslibusb32) IDNO nodriver
	ExecWait "rundll32 libusb0.dll,usb_install_driver_np_rundll $INSTDIR\bin\brltty-libusb.inf"
nodriver:
!macroend

!macro BrlttyUninstall
	ExecWait "rundll32 libusb0.dll,usb_uninstall_service_np_rundll"
	!insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED "$SYSDIR\libusb0.dll"
!macroend

!include brltty.nsi
