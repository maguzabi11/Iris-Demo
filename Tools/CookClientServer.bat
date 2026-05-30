@echo off
setlocal

call "%~dp0CookClient.bat" %*
if errorlevel 1 exit /b %ERRORLEVEL%

call "%~dp0CookServer.bat" %*
exit /b %ERRORLEVEL%
