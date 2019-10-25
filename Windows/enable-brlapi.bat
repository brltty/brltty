@echo off

setlocal
set programFolder=%~dp0

"%programFolder%bin\brltty" -I
if %ERRORLEVEL% == 0  net start BrlAPI
exit /B %ERRORLEVEL%
