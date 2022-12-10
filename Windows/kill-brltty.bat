@echo off

setlocal EnableDelayedExpansion
set programFolder=%~dp0
set pidFile="%programFolder%brltty.pid"

if exist "%pidFile%" (
   set /P pid= < %pidFile%

   if "!pid!" NEQ "" (
      taskkill /PID !pid! /F /T
      exit /B %ERRORLEVEL%
   )
)

exit /B 0
