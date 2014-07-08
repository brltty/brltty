@echo off
echo Running BRLTTY in debug mode. When done, close this window (for example,
echo by using the alt-space menu) and see %CD%\debug.log
"%~dp0bin\brltty" -n -e -ldebug -Ldebug.log >stdout.log 2>stderr.log
