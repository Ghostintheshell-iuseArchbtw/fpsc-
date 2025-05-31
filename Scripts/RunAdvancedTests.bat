@echo off
REM Advanced Testing Script for Object Pooling System
REM This script runs comprehensive tests on the new Advanced Object Pooling System

echo ====================================
echo  Advanced Object Pooling Tests
echo ====================================
echo.

set PROJECT_DIR=%~dp0..
set PROJECT_NAME=FPSGame
set TEST_CONFIG=Development
set LOG_FILE=%PROJECT_DIR%\Saved\Logs\AdvancedPoolingTests.log

REM Check if UE5 is available
if not exist "%UE5_ROOT%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" (
    echo ERROR: UnrealBuildTool.exe not found. Please set UE5_ROOT environment variable.
    pause
    exit /b 1
)

REM Create logs directory if it doesn't exist
if not exist "%PROJECT_DIR%\Saved\Logs" mkdir "%PROJECT_DIR%\Saved\Logs"

echo Running Advanced Object Pooling System Tests...
echo Log file: %LOG_FILE%
echo.

REM Run unit tests
echo [%time%] Starting Unit Tests... >> "%LOG_FILE%"
echo Running Unit Tests...
"%UE5_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%\%PROJECT_NAME%.uproject" -ExecCmds="Automation RunTests FPSGame.ObjectPooling.Unit" -unattended -nopause -nullrhi -nosplash >> "%LOG_FILE%" 2>&1

if %ERRORLEVEL% neq 0 (
    echo WARNING: Some unit tests may have failed. Check log file.
)

REM Run performance tests
echo [%time%] Starting Performance Tests... >> "%LOG_FILE%"
echo Running Performance Tests...
"%UE5_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%\%PROJECT_NAME%.uproject" -ExecCmds="Automation RunTests FPSGame.ObjectPooling.Performance" -unattended -nopause -nullrhi -nosplash >> "%LOG_FILE%" 2>&1

if %ERRORLEVEL% neq 0 (
    echo WARNING: Some performance tests may have failed. Check log file.
)

REM Run integration tests
echo [%time%] Starting Integration Tests... >> "%LOG_FILE%"
echo Running Integration Tests...
"%UE5_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%\%PROJECT_NAME%.uproject" -ExecCmds="Automation RunTests FPSGame.ObjectPooling.Integration" -unattended -nopause -nullrhi -nosplash >> "%LOG_FILE%" 2>&1

if %ERRORLEVEL% neq 0 (
    echo WARNING: Some integration tests may have failed. Check log file.
)

REM Run memory stress tests
echo [%time%] Starting Memory Stress Tests... >> "%LOG_FILE%"
echo Running Memory Stress Tests...
"%UE5_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%\%PROJECT_NAME%.uproject" -ExecCmds="Automation RunTests FPSGame.ObjectPooling.MemoryStress" -unattended -nopause -nullrhi -nosplash >> "%LOG_FILE%" 2>&1

if %ERRORLEVEL% neq 0 (
    echo WARNING: Memory stress tests may have failed. Check log file.
)

REM Generate test report
echo [%time%] Generating Test Report... >> "%LOG_FILE%"
echo Generating Test Report...
powershell -Command "& { Get-Content '%LOG_FILE%' | Select-String 'Test.*:.*\d+\.\d+' | Out-File '%PROJECT_DIR%\Saved\Logs\TestSummary.txt' -Encoding UTF8 }"

echo.
echo ====================================
echo  Testing completed!
echo ====================================
echo.
echo Test results saved to: %LOG_FILE%
echo Test summary saved to: %PROJECT_DIR%\Saved\Logs\TestSummary.txt
echo.

REM Display quick summary
if exist "%PROJECT_DIR%\Saved\Logs\TestSummary.txt" (
    echo Quick Summary:
    type "%PROJECT_DIR%\Saved\Logs\TestSummary.txt"
)

pause
