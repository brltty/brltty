Version = 0.2
ScriptName =  BRLTTY Configurator %Version%


;----------------------------------------------------------------------------------------------------------------------------------------
; Platform:         Windows XP/Vista
; Author:           Michel Such
; AutoHotkey     	Version: 1.0.47
; Version :			Alpha release 1
;
; Script Function:
;	Facility to configure Braille device and communication port
;  for Windows version of BRLTTY

#NoTrayIcon

#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

CnfFile = %A_ScriptDir%\etc\brltty.conf ; Configuration file for BRLTTY

IfNotExist, %CnfFile%
{
  Msgbox, 0, %ScriptName%, File %CnfFile% not found.
  ExitApp
}

Gosub SelectPortAndDisplay
Return

SelectPortAndDisplay:

  WinTitle = %ScrimtName%

  ; Fill a listbox for port selection
   ComPorts = USB:|COM1|COM2|COM3|COM4|COM5|COM6|COM7|COM8|COM9|COM10|
   ComPorts = %ComPorts%COM11|COM12|COM13|COM14|COM15|COM16|

  ; Fill a listbox for terminal selection
  ; getting data from brltty.conf
   Loop, read, %CnfFile%
   {
     IfInString A_LoopReadLine, #braille-driver
     {
       Term := SubStr(A_LoopReadLine, 17)
       StringReplace, Term, Term, #, -, 1
       TermList = %TermList%%Term%|
     }
   }

  ; Draw and display the window
   Gui, Font, S12 CDefault, Arial
   Gui, Add, Text, x6 y10 w250 h20, Select Braille Display
   Gui, Add, ListBox, vTerminal x6 y50 w500 h150 hscroll vscroll , %TermList%
   Gui, Add, Text,  x6 y230 w250 h20, Select communication port
   Gui, Add, ListBox, vComPort x6 y260 w500 h150 hscroll vscroll , %ComPorts%
   Gui, Add, Button, vSend gValidate x140 y430 w60 h21 +Default, &OK
   Gui, Add, Button, vCancel gGuiClose x290 y430 w60 h21, &Cancel

   Gui, Show,,   %Wintitle%

   ;Use Escape to immediately leave the program
   Escape::
        Gosub GuiClose
   return
return


;--------------------------------------------------------------------------------------------
; Functions
;------------------------------------------------------------------------------------------


Validate:
   GuiControlGet, Terminal
   if StrLen(Terminal) == 0
   {
     Msgbox 0, %ScriptName%, You must select a terminal
     Return
   }
   GuiControlGet, ComPort
   if StrLen(ComPort) == 0
   {
     Msgbox 0, %ScriptName%, You must select a communication port
     Return
   }

   Msgbox 1, %ScriptName%, You selected terminal %Terminal%`nattached to port %ComPort%
   IfMsgBox Cancel
   {
     Gosub GuiClose
   }

   IfInString ComPort, COM
   {
     FileAppend braille-device serial:%ComPort%`n, %CnfFile%
   }
   Else
   {
     FileAppend braille-device %ComPort%`n, %CnfFile%
   }
   StringReplace Terminal, Terminal, -, #
   FileAppend braille-driver %Terminal%`n, %CnfFile%

   Gosub GuiClose
return

;----------------------------------------------------------------------------------------------------------------------------------------

;End the program

GuiClose:
   Gui, cancel
   GuiEscape:
   ExitApp
