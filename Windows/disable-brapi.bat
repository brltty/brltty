@echo off

setlocal
set programFolder=%~dp0

net stop BrlAPI
"%programFolder%bin\brltty" -R
