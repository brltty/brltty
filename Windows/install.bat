@echo off
"%~dp0bin\brltty" -I
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
net start BrlAPI
