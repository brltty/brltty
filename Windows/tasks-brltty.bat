@echo off

setlocal
set programDirectory=%~dp0
call "%programDirectory%setvars-brltty"

tasklist /V /FI "imageName eq %packageName%.*"
exit /B %ERRORLEVEL%
