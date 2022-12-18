@echo off

setlocal EnableDelayedExpansion
set programFolder=%~dp0

set serviceName=BrlAPI
set serviceKey=HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\%serviceName%
set commandValue=ImagePath

sc query %serviceName% >NUL 2>NUL
if %ERRORLEVEL% NEQ 0 (
   set programName=enable
   call "%programFolder%run-brltty" -I %*
   if !ERRORLEVEL! NEQ 0  exit /B !ERRORLEVEL!

   set programName=service
   call %programFolder%set-variables
   set logLevel=info

   for /F "tokens=1,2,*" %%A in (
      'reg query %serviceKey% /V %commandValue%'
   ) do (
      if %%A == %commandValue% (
         set commandData=%%C -U "!updatableFolder!" -W "!writableFolder!" -L "!logFile!" -l "!logLevel!"
         set commandData=!commandData:"="^""!
         reg add %serviceKey% /F /V %commandValue% /D "!commandData!"
         if !ERRORLEVEL! NEQ 0  exit /B !ERRORLEVEL!
         break
      )
   )
)

net start %serviceName%
exit /B %ERRORLEVEL%
