@echo off

setlocal EnableDelayedExpansion
call "%programDirectory%setvars-brlapi"

reg query "%serviceKey%"
exit /B %ERRORLEVEL%
