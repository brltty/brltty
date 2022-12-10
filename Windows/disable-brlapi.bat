@echo off

setlocal
set programFolder=%~dp0
set programName=disable

net stop BrlAPI
call "%programFolder%run-brltty" -R %*
