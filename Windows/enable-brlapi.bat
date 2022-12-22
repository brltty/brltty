@echo off

setlocal EnableDelayedExpansion
set programDirectory=%~dp0
call "%programDirectory%setvars-brlapi"

set serviceState=
for /F "usebackq tokens=1,4" %%A in (
   `sc query "%serviceName%"`
) do (
   if "%%A" == "STATE" (
      set serviceState=%%B
      break
   )
)

if "%serviceState%" == "RUNNING" (
   @echo The %serviceName% service is already installed and running.
   exit /B 0
)

if "%serviceState%" == "" (
   set programName=enable
   call "%programDirectory%run-brltty" -I %*
   if !ERRORLEVEL! NEQ 0  exit /B !ERRORLEVEL!

   set programName=service
   call "%programDirectory%setvars-brltty"
   set logLevel=info

   for /F "usebackq tokens=1,2,*" %%A in (
      `reg query "%serviceKey%" /V "%commandValue%"`
   ) do (
      if "%%A" == "%commandValue%" (
         set commandData=%%C -U "!updatableDirectory!" -W "!writableDirectory!" -L "!logFile!" -l "!logLevel!"
         set commandData=!commandData:"="^""!
         reg add "%serviceKey%" /F /V "%commandValue%" /D "!commandData!"
         if !ERRORLEVEL! NEQ 0  exit /B !ERRORLEVEL!
         break
      )
   )
) else (
   @echo The %serviceName% service is already installed.
)

net start "%serviceName%"
exit /B %ERRORLEVEL%
