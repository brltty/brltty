@echo off
"%~dp0bin\brltty" -I
if NOT %ERRORLEVEL% == 0  exit /B %ERRORLEVEL%
net start BrlAPI
