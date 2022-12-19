@echo off

setlocal
set programDirectory=%~dp0

call "%programDirectory%setvars-brlapi"
net stop %serviceName%

set programName=disable
call "%programDirectory%run-brltty" -R %*

exit /B 0
