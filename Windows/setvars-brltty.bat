@echo off

set packageName=brltty
if "%programName%" == ""  set programName=%packageName%

set updatableDirectory=%programDirectory%etc
set writableDirectory=%programDirectory%run

set configurationFile=%programDirectory%etc\%packageName%.conf
set logFile=%programDirectory%%programName%.log
set pidFile=%writableDirectory%\%programName%.pid

exit /B 0
