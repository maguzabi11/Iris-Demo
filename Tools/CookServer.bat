@echo off
setlocal

set "PROJECT_ROOT=%~dp0.."
for %%I in ("%PROJECT_ROOT%") do set "PROJECT_ROOT=%%~fI"

set "UE_ENGINE_DIR=%UE_571_ENGINE_DIR%"
if "%UE_ENGINE_DIR%"=="" set "UE_ENGINE_DIR=G:\UE\UnrealEngine-5.7.1-release\Engine"

set "UNREAL_EDITOR_CMD=%UE_ENGINE_DIR%\Binaries\Win64\UnrealEditor-Cmd.exe"
set "PROJECT_FILE=%PROJECT_ROOT%\IrisDemo.uproject"

if not exist "%UNREAL_EDITOR_CMD%" (
    echo UnrealEditor-Cmd.exe not found: %UNREAL_EDITOR_CMD%
    echo Set UE_571_ENGINE_DIR to the UE 5.7.1 Engine directory and retry.
    exit /b 1
)

if not exist "%PROJECT_FILE%" (
    echo Project file not found: %PROJECT_FILE%
    exit /b 1
)

call "%UNREAL_EDITOR_CMD%" "%PROJECT_FILE%" -run=Cook -TargetPlatform=WindowsServer -Map=/Game/ThirdPerson/Lvl_ThirdPerson -unattended %*
exit /b %ERRORLEVEL%
