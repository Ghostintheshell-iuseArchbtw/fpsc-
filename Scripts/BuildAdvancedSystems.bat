@echo off
REM Advanced FPS Game Build Script for Windows
REM This script builds the project with focus on the new Advanced Object Pooling System

echo ====================================
echo  Advanced FPS Game Build Script
echo ====================================
echo.

REM Set project variables
set PROJECT_DIR=%~dp0..
set PROJECT_NAME=FPSGame
set BUILD_CONFIG=Development
set TARGET_PLATFORM=Win64

REM Check if UE5 is available
if not exist "%UE5_ROOT%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" (
    echo ERROR: UnrealBuildTool.exe not found. Please set UE5_ROOT environment variable.
    echo Example: set UE5_ROOT=C:\Program Files\Epic Games\UE_5.3
    pause
    exit /b 1
)

echo Building project: %PROJECT_NAME%
echo Configuration: %BUILD_CONFIG%
echo Platform: %TARGET_PLATFORM%
echo.

REM Clean previous build artifacts
echo Cleaning previous build...
if exist "%PROJECT_DIR%\Binaries" rmdir /s /q "%PROJECT_DIR%\Binaries"
if exist "%PROJECT_DIR%\Intermediate" rmdir /s /q "%PROJECT_DIR%\Intermediate"

REM Generate project files
echo Generating project files...
"%UE5_ROOT%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="%PROJECT_DIR%\%PROJECT_NAME%.uproject" -game -rocket -progress

if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to generate project files
    pause
    exit /b 1
)

REM Build the project
echo Building %PROJECT_NAME%...
"%UE5_ROOT%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" %PROJECT_NAME%Editor %TARGET_PLATFORM% %BUILD_CONFIG% -project="%PROJECT_DIR%\%PROJECT_NAME%.uproject" -rocket -progress -NoHotReloadFromIDE

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

REM Build game target
echo Building game target...
"%UE5_ROOT%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" %PROJECT_NAME% %TARGET_PLATFORM% %BUILD_CONFIG% -project="%PROJECT_DIR%\%PROJECT_NAME%.uproject" -rocket -progress

if %ERRORLEVEL% neq 0 (
    echo ERROR: Game build failed
    pause
    exit /b 1
)

echo.
echo ====================================
echo  Build completed successfully!
echo ====================================
echo.

REM Run automated tests if requested
if "%1"=="test" (
    echo Running automated tests...
    call "%~dp0RunAdvancedTests.bat"
)

pause
