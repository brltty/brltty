@echo off

setlocal
set programFolder=%~dp0

set defaultName=brltty
if x%programName% == x  set programName=%defaultName%

set pidFile=%programFolder%%programName%.pid
set logFile=%programFolder%%programName%.log

set logLevel=info
set configurationFile=%programFolder%etc\brltty.conf

if %programName% == %defaultName% (
   echo Running %programName%. When done, close this window ^(for example, by using the
   echo Alt-Space menu^). The log file is "%logFile%".
)

"%programFolder%bin\brltty" -n -P "%pidFile%" -L "%logFile%" -l "%logLevel%" -f "%configurationFile%" %*
exit /B %ERRORLEVEL%
