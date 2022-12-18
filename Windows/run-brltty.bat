@echo off

setlocal
set programFolder=%~dp0
call %programFolder%set-variables
set logLevel=info

if %programName% == %packageName% (
   echo Running %programName%. When done, close this window ^(for example, by using the
   echo Alt-Space menu^). The log file is "%logFile%".
)

"%programFolder%bin\brltty" -n -U "%updatableFolder%" -W "%writableFolder%" -P "%pidFile%" -L "%logFile%" -l "%logLevel%" -f "%configurationFile%" %*
exit /B %ERRORLEVEL%
