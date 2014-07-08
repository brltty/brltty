@echo off
net stop BrlAPI
"%~dp0bin\brltty" -R
