@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "PROJECT_ROOT=%~dp0.."
for %%I in ("%PROJECT_ROOT%") do set "PROJECT_ROOT=%%~fI"

set "BINARIES_DIR=%PROJECT_ROOT%\Binaries\Win64"
set "CLIENT_EXE=%BINARIES_DIR%\IrisDemoClient.exe"
set "SERVER_EXE=%BINARIES_DIR%\IrisDemoServer.exe"

if not exist "%SERVER_EXE%" (
    echo IrisDemoServer.exe not found: %SERVER_EXE%
    echo Build it first with Tools\BuildServer.bat.
    exit /b 1
)

if not exist "%CLIENT_EXE%" (
    echo IrisDemoClient.exe not found: %CLIENT_EXE%
    echo Build it first with Tools\BuildClient.bat.
    exit /b 1
)

:: Default settings
set "USE_IRIS=1"
set "PORT=7777"
set "DRY_RUN=0"
set "SERVER_STARTUP_WAIT=5"
set "SCENARIO_ARGS=-ScenarioASeed=1001 -ScenarioASensorCount=6 -ScenarioADroneCount=3 -ScenarioASupplyCrateCount=3 -ScenarioAUpdateInterval=2"
for /f %%I in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set "SCENARIO_RUN_ID=ScenarioA_%%I"
set "EXTRA_ARGS="

:: Command line argument parsing
:loop
if "%~1"=="" goto end_loop
set "CURRENT_ARG=%~1"
set "NEXT_ARG=%~2"
if /i "!CURRENT_ARG!"=="--no-iris" (
    set "USE_IRIS=0"
) else if /i "!CURRENT_ARG!"=="--iris" (
    set "USE_IRIS=1"
) else if /i "!CURRENT_ARG!"=="-UseIrisReplication" (
    if "%~2"=="0" (
        set "USE_IRIS=0"
        shift
    ) else if "%~2"=="1" (
        set "USE_IRIS=1"
        shift
    )
) else if /i "!CURRENT_ARG!"=="-UseIrisReplication=0" (
    set "USE_IRIS=0"
) else if /i "!CURRENT_ARG!"=="-UseIrisReplication=1" (
    set "USE_IRIS=1"
) else if /i "!CURRENT_ARG!"=="--dry-run" (
    set "DRY_RUN=1"
) else if /i "!CURRENT_ARG!"=="--server-wait-timeout" (
    set "SERVER_STARTUP_WAIT=%~2"
    shift
) else if /i "!CURRENT_ARG:~0,22!"=="--server-wait-timeout=" (
    set "SERVER_STARTUP_WAIT=!CURRENT_ARG:~22!"
) else if /i "!CURRENT_ARG!"=="--server-startup-wait" (
    set "SERVER_STARTUP_WAIT=%~2"
    shift
) else if /i "!CURRENT_ARG:~0,22!"=="--server-startup-wait=" (
    set "SERVER_STARTUP_WAIT=!CURRENT_ARG:~22!"
) else if /i "!CURRENT_ARG!"=="--run-id" (
    set "SCENARIO_RUN_ID=%~2"
    shift
) else (
    if not "!CURRENT_ARG!"=="" if "!CURRENT_ARG:~0,1!"=="-" if not "!NEXT_ARG!"=="" if not "!NEXT_ARG:~0,1!"=="-" (
        set "EXTRA_ARGS=!EXTRA_ARGS! !CURRENT_ARG!=!NEXT_ARG!"
        shift
    ) else (
        set "EXTRA_ARGS=!EXTRA_ARGS! !CURRENT_ARG!"
    )
)
shift
goto loop
:end_loop

set "IRIS_ARG=-UseIrisReplication=%USE_IRIS% -net.Iris.UseIrisReplication=%USE_IRIS%"
set "RUN_ARG=-ScenarioARunId=%SCENARIO_RUN_ID%"
set "ALL_ARGS=%IRIS_ARG% %RUN_ARG% %SCENARIO_ARGS% %EXTRA_ARGS%"

echo ===================================================
echo [IrisDemo] Local Binaries Multiplay Test Session
echo ===================================================
echo Binaries Dir: %BINARIES_DIR%
echo Server:       %SERVER_EXE%
echo Client:       %CLIENT_EXE%
echo Iris Enabled: %USE_IRIS%
echo Port:         %PORT%
echo Run Id:       %SCENARIO_RUN_ID%
echo Dry Run:      %DRY_RUN%
echo Server Wait:  %SERVER_STARTUP_WAIT%s
echo Base Args:    %SCENARIO_ARGS%
echo Extra Args:   %EXTRA_ARGS%
echo ===================================================

if "%DRY_RUN%"=="1" (
    echo Server Command: start "IrisDemo Binary Dedicated Server" "%SERVER_EXE%" -log -stdout -FullStdOutLogOutput -port=%PORT% %ALL_ARGS%
    echo Client Command: "%CLIENT_EXE%" 127.0.0.1:%PORT% -log -windowed -resx=960 -resy=540 [window position] %ALL_ARGS%
    echo Dry run complete. No sessions launched.
    exit /b 0
)

echo Starting Dedicated Server...
start "IrisDemo Binary Dedicated Server" "%SERVER_EXE%" Lvl_ThirdPerson -log -stdout -FullStdOutLogOutput -port=%PORT% %ALL_ARGS%

if "%SERVER_STARTUP_WAIT%"=="0" (
    echo Server wait disabled. Starting clients immediately.
    goto server_ready
)

echo Waiting %SERVER_STARTUP_WAIT% seconds for dedicated server startup...
set /A SERVER_STARTUP_PINGS=SERVER_STARTUP_WAIT + 1
ping 127.0.0.1 -n %SERVER_STARTUP_PINGS% > nul

:server_ready

echo Starting Client 1 (Left-Top)...
start "IrisDemo Binary Client 1" "%CLIENT_EXE%" 127.0.0.1:%PORT% -log -windowed -resx=960 -resy=540 -WinX=0 -WinY=0 %ALL_ARGS%

echo Starting Client 2 (Right-Top)...
start "IrisDemo Binary Client 2" "%CLIENT_EXE%" 127.0.0.1:%PORT% -log -windowed -resx=960 -resy=540 -WinX=960 -WinY=0 %ALL_ARGS%

echo Starting Client 3 (Left-Bottom)...
start "IrisDemo Binary Client 3" "%CLIENT_EXE%" 127.0.0.1:%PORT% -log -windowed -resx=960 -resy=540 -WinX=0 -WinY=540 %ALL_ARGS%

echo All binary sessions launched.
endlocal
