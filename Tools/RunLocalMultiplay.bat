@echo off
setlocal

set "PROJECT_ROOT=%~dp0.."
for %%I in ("%PROJECT_ROOT%") do set "PROJECT_ROOT=%%~fI"

set "UE_ENGINE_DIR=%UE_571_ENGINE_DIR%"
if "%UE_ENGINE_DIR%"=="" set "UE_ENGINE_DIR=G:\UE\UnrealEngine-5.7.1-release\Engine"

set "UNREAL_EDITOR=%UE_ENGINE_DIR%\Binaries\Win64\UnrealEditor.exe"
set "PROJECT_FILE=%PROJECT_ROOT%\IrisDemo.uproject"

if not exist "%UNREAL_EDITOR%" (
    echo UnrealEditor.exe not found: %UNREAL_EDITOR%
    echo Set UE_571_ENGINE_DIR to the UE 5.7.1 Engine directory and retry.
    exit /b 1
)

if not exist "%PROJECT_FILE%" (
    echo Project file not found: %PROJECT_FILE%
    exit /b 1
)

:: 기본 설정값
set "USE_IRIS=1"
set "PORT=7777"
set "SCENARIO_ARGS=-ScenarioASeed=1001 -ScenarioASensorCount=6 -ScenarioADroneCount=3 -ScenarioASupplyCrateCount=3 -ScenarioAUpdateInterval=2"
for /f %%I in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set "SCENARIO_RUN_ID=ScenarioA_%%I"
set "EXTRA_ARGS="

:: 명령행 인자 분석 루프
:loop
if "%~1"=="" goto end_loop
if /i "%~1"=="--no-iris" (
    set "USE_IRIS=0"
) else if /i "%~1"=="--iris" (
    set "USE_IRIS=1"
) else if /i "%~1"=="-UseIrisReplication=0" (
    set "USE_IRIS=0"
) else if /i "%~1"=="-UseIrisReplication=1" (
    set "USE_IRIS=1"
) else if /i "%~1"=="--run-id" (
    set "SCENARIO_RUN_ID=%~2"
    shift
) else (
    set "EXTRA_ARGS=%EXTRA_ARGS% %1"
)
shift
goto loop
:end_loop

set "IRIS_ARG=-UseIrisReplication=%USE_IRIS% -net.Iris.UseIrisReplication=%USE_IRIS%"
set "RUN_ARG=-ScenarioARunId=%SCENARIO_RUN_ID%"
set "ALL_ARGS=%IRIS_ARG% %RUN_ARG% %SCENARIO_ARGS% %EXTRA_ARGS%"

echo ===================================================
echo [IrisDemo] Local Multiplay Test Session
echo ===================================================
echo Engine Dir: %UE_ENGINE_DIR%
echo Project:    %PROJECT_FILE%
echo Iris Enabled: %USE_IRIS%
echo Port:       %PORT%
echo Run Id:     %SCENARIO_RUN_ID%
echo Base Args:  %SCENARIO_ARGS%
echo Extra Args: %EXTRA_ARGS%
echo ===================================================

echo Starting Dedicated Server...
start "IrisDemo Dedicated Server" "%UNREAL_EDITOR%" "%PROJECT_FILE%" Lvl_ThirdPerson -server -log -port=%PORT% %ALL_ARGS%

:: 서버가 바인딩될 시간을 벌기 위한 대기 (2초)
ping 127.0.0.1 -n 3 > nul

echo Starting Client 1 (Left-Top)...
start "IrisDemo Client 1" "%UNREAL_EDITOR%" "%PROJECT_FILE%" 127.0.0.1:%PORT% -game -log -windowed -resx=960 -resy=540 -WinX=0 -WinY=0 %ALL_ARGS%

echo Starting Client 2 (Right-Top)...
start "IrisDemo Client 2" "%UNREAL_EDITOR%" "%PROJECT_FILE%" 127.0.0.1:%PORT% -game -log -windowed -resx=960 -resy=540 -WinX=960 -WinY=0 %ALL_ARGS%

echo Starting Client 3 (Left-Bottom)...
start "IrisDemo Client 3" "%UNREAL_EDITOR%" "%PROJECT_FILE%" 127.0.0.1:%PORT% -game -log -windowed -resx=960 -resy=540 -WinX=0 -WinY=540 %ALL_ARGS%

echo All sessions launched.
endlocal
