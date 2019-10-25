@echo off

setlocal
set programFolder=%~dp0
set logLevel=debug,serial,usb,bluetooth,inpkts,outpkts,brldrv

"%programFolder%run-brltty" "-l%logLevel%" %*
exit /B %ERRORLEVEL%
