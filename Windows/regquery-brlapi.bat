@echo off

setlocal EnableDelayedExpansion

set serviceName=BrlAPI
set serviceKey=HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\%serviceName%

reg query %serviceKey%
exit /B %ERRORLEVEL%
