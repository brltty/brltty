Version = 0.5


;----------------------------------------------------------------------------------------------------------------------------------------
; Platform:         Windows XP/Vista
; Author:           Michel Such
; AutoHotkey     	Version: 1.0.47
; Version :			Alpha release 1
;
; Script Function:
;	Facility to configure Braille device and communication port
;  for Windows version of BRLTTY

#SingleInstance
#NoTrayIcon

#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

CnfFile = %A_ScriptDir%\etc\brltty.conf ; Configuration file for BRLTTY
Gosub SetLanguage

IfNotExist, %CnfFile%
{
  Msgbox, 0, %ScriptName%, %CnfFileNotFoundMsg%
  ExitApp
}

Gosub SelectPortAndDisplay
Return

SelectPortAndDisplay:

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
   Gui, Add, Text, x6 y6 w800 h160, %HelpText%

   Gui, Add, Text, x6 y166 w400 h20, %SelectBrailleDisplayText%
   Gui, Add, ListBox, vTerminal x6 y190 w400 h150 hscroll vscroll , %TermList%

   Gui, Add, Text,  x412 y166 w400 h20, %SelectBraillePortText%
   Gui, Add, ListBox, vComPort x412 y190 w400 h150 hscroll vscroll , %ComPorts%
   Gui, Add, Button, vSend gValidate x640 y352 w100 h24 +Default, %OKBtn%
   Gui, Add, Button, vCancel gGuiClose x750 y352 w100 h24, %CancelBtn%

   Gui, Show, AutoSize Center,   %ScriptName%

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
   if Terminal =
   {
     Msgbox 0, %ScriptName%, %NeedToChooseAFileMsg%
     ControlFocus, %SelectBrailleDisplayText%, %ScriptName%
     Return
   }
   GuiControlGet, ComPort
   if ComPort =
   {
     Msgbox 0, %ScriptName%, %NeedToChooseAPortMsg%
     ControlFocus, %SelectBraillePortText%, %ScriptName%
     Return
   }

   SetTimer, ChangeButtonNames, 50
   Msgbox 1, %ScriptName%, %YouHaveSelectedMsg% %Terminal%`n%ConnectedToPortMsg% %ComPort%
   IfMsgBox OK
   {
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

     ; Restart BRLTTY if the -r parameter was passed
     If 1 = -r
     {
       SplitPath, ComSpec,, SysDir
       ; Stop BrlAPI and BRLTTY if started
       RunWait, %SysDir%\net.exe stop BrlApi
       RunWait, %A_ScriptDir%\bin\brltty.exe -R

       ; Restart BRLTTY and BrlAPI
       RunWait, %A_ScriptDir%\bin\brltty.exe -I
       RunWait, %SysDir%\net.exe start BrlApi
     }
     Gosub GuiClose
   }
   ControlFocus, %SelectBrailleDisplayText%, %ScriptName%
return

;----------------------------------------------------------------------------------------------------------------------------------------

;End the program

GuiClose:
   Gui, cancel
   GuiEscape:
   ExitApp

;----------------------------------------------------------------------------------------------------------------------------------------
; Support for different languages

SetLanguage:

; This procedure uses the AutoHotkey A_Language variable
; which contains the language code of the system the program is running on.
; To get a list of A_Language values and corresponding languages, goto:
; http://www.autohotkey.com/docs/misc/Languages.htm

  languageCode_040c = French
  languageCode_080c = French
  languageCode_0c0c = French
  languageCode_100c = French
  languageCode_140c = French
  languageCode_180c = French

  languageCode_0416 = Portuguese
  languageCode_0816 = Portuguese

  the_language := languageCode_%A_Language%  ; Get the name of the system's default language.
  if the_language =
  {
    the_language := "English"
  }
  Gosub %the_language%
Return


ChangeButtonNames:
  IfWinNotExist, %ScriptName%
 	return  ; Keep waiting.
  SetTimer, ChangeButtonNames, off
  WinActivate
  ControlSetText, Button1, %OKBtn%
ControlSetText, Button2, %CancelBtn%
return

English: ; English translation (default)

  ScriptName =  BRLTTY Configurator %Version%
  CnfFileNotFoundMsg = File %CnfFile% not found.
  NeedToChooseAFileMsg = You must select a braille display
  NeedToChooseAPortMsg = You must select a communication port
  YouHaveSelectedMsg = You selected terminal
  ConnectedToPortMsg = attached to port

  HelpText =
  (
  Please choose the braille display type and communication port.
  - If your braille device is USB,
    * if you have installed your manufacturer's driver, you should rather select its virtual COM port if it provides one,
      else select "USB:" to use libusb-win32's filter,
    * if you do not want or can not install your manufacturer's driver, you can select "USB:" here and install
      libusb-win32's driver.
  - If your braille device is serial or is connect through a serial to USB converter, just select the proper COM port.
    Make sure to select the proper braille display as incorrect probing seems to possibly brick some devices.
  )

  SelectBrailleDisplayText = Select braille display
  SelectBraillePortText = Select communication port

  OKBtn = &OK
  CancelBtn = &Cancel
Return

French: ; French translation

  ScriptName =  Configurateur BRLTTY %Version%
  CnfFileNotFoundMsg = Le fichier %CnfFile% est introuvable
  NeedToChooseAFileMsg = Vous devez choisir un afficheur braille
  NeedToChooseAPortMsg = Vous devez choisir un port de communication
  YouHaveSelectedMsg = Vous avez choisi l'afficheur
  ConnectedToPortMsg = connecté au port

  HelpText =
  (
  Veuillez choisir le type d'afficheur braille et le port de communication.
  - Si votre afficheur utilise un port USB,
    * si vous avez installé le pilote de votre fabriquant, vous devriez choisir son port COM virtuel s'il en fournit un, sinon
      choisissez "USB:" pour utiliser le filtre libusb-win32,
    * si vous ne voulez pas ou ne pouvez pas installer le pilote de votre fabriquant, vous pouvez choisir "USB:" ici et
      installer le pilote libusb-win32.
  - Si votre afficheur braille utilise un port série ou est connecté via un adaptateur USB vers série, choisissez simplement le port
    COM approprié. Assurez-vous de choisir le bon afficheur car un choix incorrect semble parfois planter certains matériels.
  )

  SelectBrailleDisplayText = Choisissez l'afficheur braille
  SelectBraillePortText = Choisissez le port de communication

  OKBtn = &Accepter
  CancelBtn = A&nnuler
Return


Portuguese: ; Portuguese translation

  ScriptName =  Configurador do BRLTTY %Version%
  CnfFileNotFoundMsg = O ficheiro %CnfFile% não foi encontrado
  NeedToChooseAFileMsg = Tem que seleccionar uma linha Braille
  NeedToChooseAPortMsg = Tem que seleccionar uma porta de mcomunicação
  YouHaveSelectedMsg = Seleccionou o terminal
  ConnectedToPortMsg = conectado à porta

  HelpText =
  (
  Por favor escolha o tipo da linha Braille e a porta de comunicação.
  - Se o seu dispositivo Braille é USB,
  * se tem instalado o driver do fabricante, deverá seleccionar a porta de série respectiva se existir, caso contrário
  	seleccione "USB:" para usar o filtro libusb-win32's.
  * se não pretende ou não consegue instalar o driver do fabricante, pode seleccionar aqui "USB:" e instalar
  	o driver libusb-win32's.
  - Se o seu dispositivo braille trabalha via porta série, seleccione a apropriada. Tenha a certeza que selecciona a linha Braille correcta visto que
  	utilizar drivers errados poderá danificar alguns dispositivos...
  )

  SelectBrailleDisplayText = Seleccione a linha Braille
  SelectBraillePortText = Seleccione a porta de comunicação

  OKBtn = &OK
  CancelBtn = &Cancelar
Return
