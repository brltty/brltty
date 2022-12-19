@echo off

setlocal
set programDirectory=%~dp0
call "%programDirectory%setvars-brltty"
set logLevel=info

if "%programName%" == "%packageName%" (
   echo Running %programName%. When done, close this window ^(for example, by using the
   echo Alt-Space menu^). The log file is "%logFile%".
)

"%programDirectory%bin\brltty" -n -N -U "%updatableDirectory%" -W "%writableDirectory%" -P "%pidFile%" -L "%logFile%" -l "%logLevel%" -f "%configurationFile%" %*
exit /B %ERRORLEVEL%
