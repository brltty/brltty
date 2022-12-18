@echo off

setlocal EnableDelayedExpansion
set programFolder=%~dp0
call %programFolder%set-variables

if exist "%pidFile%" (
   set /P pid= < %pidFile%

   if "!pid!" NEQ "" (
      taskkill /PID !pid! /F /T
      exit /B %ERRORLEVEL%
   )
)

exit /B 0
