@echo off

set packageName=brltty
if x%programName% == x  set programName=%packageName%

set updatableFolder=%programFolder%etc
set writableFolder=%programFolder%run

set configurationFile=%programFolder%etc\%packageName%.conf
set logFile=%programFolder%%programName%.log
set pidFile=%writableFolder%\%programName%.pid

