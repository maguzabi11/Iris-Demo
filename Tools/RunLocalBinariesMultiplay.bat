@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "PROJECT_ROOT=%~dp0.."
for %%I in ("%PROJECT_ROOT%") do set "PROJECT_ROOT=%%~fI"

set "BINARIES_DIR=%PROJECT_ROOT%\Binaries\Win64"
set "CLIENT_EXE=%BINARIES_DIR%\IrisDemoClient.exe"
set "SERVER_EXE=%BINARIES_DIR%\IrisDemoServer.exe"
set "SERVER_LOG=%PROJECT_ROOT%\Saved\Logs\RunLocalBinariesServer.log"

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
set "SERVER_WAIT_TIMEOUT=60"
set "SCENARIO_ARGS=-ScenarioASeed=1001 -ScenarioASensorCount=6 -ScenarioADroneCount=3 -ScenarioASupplyCrateCount=3 -ScenarioAUpdateInterval=2"
set "EXTRA_ARGS="

:: Command line argument parsing
:loop
if "%~1"=="" goto end_loop
if /i "%~1"=="--no-iris" (
    set "USE_IRIS=0"
) else if /i "%~1"=="--iris" (
    set "USE_IRIS=1"
) else if /i "%~1"=="-UseIrisReplication" (
    if "%~2"=="0" (
        set "USE_IRIS=0"
        shift
    ) else if "%~2"=="1" (
        set "USE_IRIS=1"
        shift
    )
) else if /i "%~1"=="-UseIrisReplication=0" (
    set "USE_IRIS=0"
) else if /i "%~1"=="-UseIrisReplication=1" (
    set "USE_IRIS=1"
) else if /i "%~1"=="--dry-run" (
    set "DRY_RUN=1"
) else if /i "%~1"=="--server-wait-timeout" (
    set "SERVER_WAIT_TIMEOUT=%~2"
    shift
) else if /i "%~1"=="--server-wait-timeout=0" (
    set "SERVER_WAIT_TIMEOUT=0"
) else (
    set "CURRENT_ARG=%~1"
    set "NEXT_ARG=%~2"
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

set "IRIS_ARG=-UseIrisReplication=%USE_IRIS%"
set "ALL_ARGS=%IRIS_ARG% %SCENARIO_ARGS% %EXTRA_ARGS%"

echo ===================================================
echo [IrisDemo] Local Binaries Multiplay Test Session
echo ===================================================
echo Binaries Dir: %BINARIES_DIR%
echo Server:       %SERVER_EXE%
echo Client:       %CLIENT_EXE%
echo Server Log:   %SERVER_LOG%
echo Iris Enabled: %USE_IRIS%
echo Port:         %PORT%
echo Dry Run:      %DRY_RUN%
echo Server Wait:  %SERVER_WAIT_TIMEOUT%s
echo Base Args:    %SCENARIO_ARGS%
echo Extra Args:   %EXTRA_ARGS%
echo ===================================================

if "%DRY_RUN%"=="1" (
    echo Server Command: powershell -NoProfile -NoExit -ExecutionPolicy Bypass -Command "^& '%SERVER_EXE%' -log -stdout -FullStdOutLogOutput -port=%PORT% %ALL_ARGS% 2^>^&1 ^| Tee-Object -FilePath '%SERVER_LOG%'"
    echo Client Command: "%CLIENT_EXE%" 127.0.0.1:%PORT% -log -windowed -resx=960 -resy=540 [window position] %ALL_ARGS%
    echo Dry run complete. No sessions launched.
    exit /b 0
)

echo Starting Dedicated Server...
start "IrisDemo Binary Dedicated Server" powershell -NoProfile -NoExit -ExecutionPolicy Bypass -Command "^& '%SERVER_EXE%' -log -stdout -FullStdOutLogOutput -port=%PORT% %ALL_ARGS% 2^>^&1 ^| Tee-Object -FilePath '%SERVER_LOG%'"

if "%SERVER_WAIT_TIMEOUT%"=="0" (
    echo Server wait disabled. Starting clients immediately.
    goto server_ready
)

echo Waiting for dedicated server UDP port %PORT%...
for /L %%S in (1,1,%SERVER_WAIT_TIMEOUT%) do (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "if (Get-NetUDPEndpoint -LocalPort %PORT% -ErrorAction SilentlyContinue) { exit 0 } exit 1" > nul 2> nul
    if !ERRORLEVEL! EQU 0 (
        echo Dedicated server is ready on UDP port %PORT%.
        goto server_ready
    )
    ping 127.0.0.1 -n 2 > nul
)

echo Timed out waiting for dedicated server UDP port %PORT% after %SERVER_WAIT_TIMEOUT% seconds.
echo Check the server window or Saved\Logs for startup errors.
exit /b 1

:server_ready

echo Starting Client 1 (Left-Top)...
start "IrisDemo Binary Client 1" "%CLIENT_EXE%" 127.0.0.1:%PORT% -log -windowed -resx=960 -resy=540 -WinX=0 -WinY=0 %ALL_ARGS%

echo Starting Client 2 (Right-Top)...
start "IrisDemo Binary Client 2" "%CLIENT_EXE%" 127.0.0.1:%PORT% -log -windowed -resx=960 -resy=540 -WinX=960 -WinY=0 %ALL_ARGS%

echo Starting Client 3 (Left-Bottom)...
start "IrisDemo Binary Client 3" "%CLIENT_EXE%" 127.0.0.1:%PORT% -log -windowed -resx=960 -resy=540 -WinX=0 -WinY=540 %ALL_ARGS%

echo All binary sessions launched.
endlocal
