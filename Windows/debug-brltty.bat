@echo off

setlocal
set programDirectory=%~dp0
set logLevel=debug,serial,usb,bt,inpkts,outpkts,brldrv,scrdrv

call "%programDirectory%run-brltty" -l "%logLevel%" %*
exit /B %ERRORLEVEL%
