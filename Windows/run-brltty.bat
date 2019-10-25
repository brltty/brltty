@echo off

setlocal
set programFolder=%~dp0
set logFile=%programFolder%brltty.log
set logLevel=info

echo Running BRLTTY. When done, close this window ^(for example, by using the
echo Alt-Space menu^). The log file is: %logFile%

"%programFolder%bin\brltty" -n "-L%logFile%" "-l%logLevel%" %*
exit /B %ERRORLEVEL%
