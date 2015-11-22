@echo off
setlocal

set programDirectory=%~dp0
set logFile=%programDirectory%debug.log
set logLevel=debug

echo Running BRLTTY in debug mode. When done, close this window (for example, by
echo using the alt-space menu) and see %logFile%

"%programDirectory%bin\brltty" -n "-l%logLevel%" "-L%logFile%" %*
