@echo off

setlocal EnableDelayedExpansion
set programFolder=%~dp0
set programName=enable

set serviceName=BrlAPI
set serviceKey=HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\%serviceName%
set serviceCommand=ImagePath
set serviceLogFile=%programFolder%service.log
set serviceLogLevel=info

sc query %serviceName% >NUL 2>NUL
if %ERRORLEVEL% NEQ 0 (
   call "%programFolder%run-brltty" -I %*
   if !ERRORLEVEL! NEQ 0  exit /B !ERRORLEVEL!

   for /F "tokens=1,2,*" %%A in (
      'reg query %serviceKey% /V %serviceCommand%'
   ) do (
      if %%A == %serviceCommand% (
         set serviceData=%%C -l "%serviceLogLevel%" -L "%serviceLogFile%"
         set serviceData=!serviceData:"="^""!
         reg add %serviceKey% /F /V %serviceCommand% /D "!serviceData!"
         if !ERRORLEVEL! NEQ 0  exit /B !ERRORLEVEL!
         break
      )
   )
)

net start %serviceName%
exit /B %ERRORLEVEL%
