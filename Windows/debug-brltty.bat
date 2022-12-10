@echo off

setlocal
set programFolder=%~dp0
set logLevel=debug,serial,usb,bt,inpkts,outpkts,brldrv

call "%programFolder%run-brltty" -l "%logLevel%" %*
exit /B %ERRORLEVEL%
