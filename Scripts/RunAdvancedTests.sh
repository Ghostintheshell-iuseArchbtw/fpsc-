#!/bin/bash
# Advanced Testing Script for Object Pooling System
# This script runs comprehensive tests on the new Advanced Object Pooling System

echo "===================================="
echo " Advanced Object Pooling Tests"
echo "===================================="
echo

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_NAME="FPSGame"
TEST_CONFIG="Development"
LOG_FILE="$PROJECT_DIR/Saved/Logs/AdvancedPoolingTests.log"

# Check if UE5 is available
if [ -z "$UE5_ROOT" ]; then
    echo "ERROR: UE5_ROOT environment variable not set."
    exit 1
fi

EDITOR_PATH="$UE5_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd"
if [[ "$OSTYPE" == "darwin"* ]]; then
    EDITOR_PATH="$UE5_ROOT/Engine/Binaries/Mac/UnrealEditor-Cmd"
fi

if [ ! -f "$EDITOR_PATH" ]; then
    echo "ERROR: UnrealEditor-Cmd not found at $EDITOR_PATH"
    exit 1
fi

# Create logs directory if it doesn't exist
mkdir -p "$PROJECT_DIR/Saved/Logs"

echo "Running Advanced Object Pooling System Tests..."
echo "Log file: $LOG_FILE"
echo

# Function to run test and log results
run_test() {
    local test_name="$1"
    local test_command="$2"
    
    echo "[$(date '+%T')] Starting $test_name..." >> "$LOG_FILE"
    echo "Running $test_name..."
    
    "$EDITOR_PATH" "$PROJECT_DIR/$PROJECT_NAME.uproject" -ExecCmds="$test_command" -unattended -nopause -nullrhi -nosplash >> "$LOG_FILE" 2>&1
    
    local exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo "WARNING: Some $test_name may have failed. Check log file."
    fi
    
    return $exit_code
}

# Run unit tests
run_test "Unit Tests" "Automation RunTests FPSGame.ObjectPooling.Unit"

# Run performance tests
run_test "Performance Tests" "Automation RunTests FPSGame.ObjectPooling.Performance"

# Run integration tests
run_test "Integration Tests" "Automation RunTests FPSGame.ObjectPooling.Integration"

# Run memory stress tests
run_test "Memory Stress Tests" "Automation RunTests FPSGame.ObjectPooling.MemoryStress"

# Generate test report
echo "[$(date '+%T')] Generating Test Report..." >> "$LOG_FILE"
echo "Generating Test Report..."
grep -E 'Test.*:.*[0-9]+\.[0-9]+' "$LOG_FILE" > "$PROJECT_DIR/Saved/Logs/TestSummary.txt" 2>/dev/null

echo
echo "===================================="
echo " Testing completed!"
echo "===================================="
echo
echo "Test results saved to: $LOG_FILE"
echo "Test summary saved to: $PROJECT_DIR/Saved/Logs/TestSummary.txt"
echo

# Display quick summary
if [ -f "$PROJECT_DIR/Saved/Logs/TestSummary.txt" ]; then
    echo "Quick Summary:"
    cat "$PROJECT_DIR/Saved/Logs/TestSummary.txt"
fi
