#!/bin/bash
# Advanced FPS Game Build Script for Linux/macOS
# This script builds the project with focus on the new Advanced Object Pooling System

echo "===================================="
echo " Advanced FPS Game Build Script"
echo "===================================="
echo

# Set project variables
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_NAME="FPSGame"
BUILD_CONFIG="Development"
TARGET_PLATFORM="Linux"

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    TARGET_PLATFORM="Mac"
fi

# Check if UE5 is available
if [ -z "$UE5_ROOT" ]; then
    echo "ERROR: UE5_ROOT environment variable not set."
    echo "Please set it to your Unreal Engine installation directory."
    echo "Example: export UE5_ROOT=/opt/UnrealEngine"
    exit 1
fi

UBT_PATH="$UE5_ROOT/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool"
if [ ! -f "$UBT_PATH" ]; then
    echo "ERROR: UnrealBuildTool not found at $UBT_PATH"
    exit 1
fi

echo "Building project: $PROJECT_NAME"
echo "Configuration: $BUILD_CONFIG"
echo "Platform: $TARGET_PLATFORM"
echo

# Clean previous build artifacts
echo "Cleaning previous build..."
rm -rf "$PROJECT_DIR/Binaries"
rm -rf "$PROJECT_DIR/Intermediate"

# Generate project files
echo "Generating project files..."
"$UBT_PATH" -projectfiles -project="$PROJECT_DIR/$PROJECT_NAME.uproject" -game -rocket -progress

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to generate project files"
    exit 1
fi

# Build the project
echo "Building $PROJECT_NAME..."
"$UBT_PATH" "${PROJECT_NAME}Editor" "$TARGET_PLATFORM" "$BUILD_CONFIG" -project="$PROJECT_DIR/$PROJECT_NAME.uproject" -rocket -progress -NoHotReloadFromIDE

if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

# Build game target
echo "Building game target..."
"$UBT_PATH" "$PROJECT_NAME" "$TARGET_PLATFORM" "$BUILD_CONFIG" -project="$PROJECT_DIR/$PROJECT_NAME.uproject" -rocket -progress

if [ $? -ne 0 ]; then
    echo "ERROR: Game build failed"
    exit 1
fi

echo
echo "===================================="
echo " Build completed successfully!"
echo "===================================="
echo

# Run automated tests if requested
if [ "$1" == "test" ]; then
    echo "Running automated tests..."
    "$SCRIPT_DIR/RunAdvancedTests.sh"
fi
