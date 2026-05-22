@echo off
setlocal

set "PROJECT_ROOT=%~dp0.."
for %%I in ("%PROJECT_ROOT%") do set "PROJECT_ROOT=%%~fI"

set "UE_ENGINE_DIR=%UE_571_ENGINE_DIR%"
if "%UE_ENGINE_DIR%"=="" set "UE_ENGINE_DIR=G:\UE\UnrealEngine-5.7.1-release\Engine"

set "BUILD_BAT=%UE_ENGINE_DIR%\Build\BatchFiles\Build.bat"
set "PROJECT_FILE=%PROJECT_ROOT%\IrisDemo.uproject"

if not exist "%BUILD_BAT%" (
    echo Build.bat not found: %BUILD_BAT%
    echo Set UE_571_ENGINE_DIR to the UE 5.7.1 Engine directory and retry.
    exit /b 1
)

if not exist "%PROJECT_FILE%" (
    echo Project file not found: %PROJECT_FILE%
    exit /b 1
)

call "%BUILD_BAT%" IrisDemoClient Win64 Development -Project="%PROJECT_FILE%" -WaitMutex -NoHotReload %*
exit /b %ERRORLEVEL%
