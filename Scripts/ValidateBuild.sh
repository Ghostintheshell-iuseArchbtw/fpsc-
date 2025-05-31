#!/bin/bash
# Comprehensive Build Validation Script
# This script performs local validation of the Advanced Object Pooling System

echo "=========================================="
echo " Advanced FPS Game Build Validation"
echo "=========================================="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Validation results
VALIDATION_PASSED=0
VALIDATION_WARNINGS=0
VALIDATION_ERRORS=0

# Function to log results
log_result() {
    local level="$1"
    local message="$2"
    
    case $level in
        "PASS")
            echo -e "${GREEN}✓ PASS:${NC} $message"
            ((VALIDATION_PASSED++))
            ;;
        "WARN")
            echo -e "${YELLOW}⚠ WARN:${NC} $message"
            ((VALIDATION_WARNINGS++))
            ;;
        "ERROR")
            echo -e "${RED}✗ ERROR:${NC} $message"
            ((VALIDATION_ERRORS++))
            ;;
        "INFO")
            echo -e "${BLUE}ℹ INFO:${NC} $message"
            ;;
    esac
}

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

log_result "INFO" "Project directory: $PROJECT_DIR"
log_result "INFO" "Starting comprehensive validation..."

echo
echo "1. Project Structure Validation"
echo "================================"

# Check essential files
if [ -f "$PROJECT_DIR/FPSGame.uproject" ]; then
    log_result "PASS" "Project file exists"
else
    log_result "ERROR" "Project file missing: FPSGame.uproject"
fi

if [ -f "$PROJECT_DIR/Source/FPSGame/FPSGame.Build.cs" ]; then
    log_result "PASS" "Build configuration exists"
else
    log_result "ERROR" "Build configuration missing"
fi

# Check Advanced Object Pooling System files
if [ -f "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.h" ]; then
    log_result "PASS" "Object Pool Manager header exists"
else
    log_result "ERROR" "Object Pool Manager header missing"
fi

if [ -f "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.cpp" ]; then
    log_result "PASS" "Object Pool Manager implementation exists"
else
    log_result "ERROR" "Object Pool Manager implementation missing"
fi

# Check Testing System files
if [ -f "$PROJECT_DIR/Source/FPSGame/Testing/AdvancedObjectPoolTests.h" ]; then
    log_result "PASS" "Advanced testing framework header exists"
else
    log_result "ERROR" "Advanced testing framework header missing"
fi

if [ -f "$PROJECT_DIR/Source/FPSGame/Testing/AdvancedObjectPoolTests.cpp" ]; then
    log_result "PASS" "Advanced testing framework implementation exists"
else
    log_result "ERROR" "Advanced testing framework implementation missing"
fi

# Check build scripts
if [ -f "$PROJECT_DIR/Scripts/BuildAdvancedSystems.sh" ] && [ -x "$PROJECT_DIR/Scripts/BuildAdvancedSystems.sh" ]; then
    log_result "PASS" "Linux build script exists and is executable"
else
    log_result "WARN" "Linux build script missing or not executable"
fi

if [ -f "$PROJECT_DIR/Scripts/RunAdvancedTests.sh" ] && [ -x "$PROJECT_DIR/Scripts/RunAdvancedTests.sh" ]; then
    log_result "PASS" "Linux test script exists and is executable"
else
    log_result "WARN" "Linux test script missing or not executable"
fi

echo
echo "2. Code Quality Validation"
echo "=========================="

# Check for proper header guards
HEADER_FILES=$(find "$PROJECT_DIR/Source" -name "*.h" 2>/dev/null)
for header in $HEADER_FILES; do
    if grep -q "#pragma once" "$header"; then
        log_result "PASS" "Header guard found in $(basename "$header")"
    else
        log_result "WARN" "No #pragma once in $(basename "$header")"
    fi
done

# Check for UCLASS declarations in appropriate files
if grep -q "UCLASS(" "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.h"; then
    log_result "PASS" "UCLASS declaration found in ObjectPoolManager"
else
    log_result "WARN" "UCLASS declaration not found in ObjectPoolManager"
fi

# Check for proper include statements
if grep -q "#include \"CoreMinimal.h\"" "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.h"; then
    log_result "PASS" "CoreMinimal.h included in ObjectPoolManager"
else
    log_result "WARN" "CoreMinimal.h not included in ObjectPoolManager"
fi

# Check for thread safety mechanisms
if grep -q "FCriticalSection\|FRWLock\|TAtomic\|FScopeLock" "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.cpp" || 
   grep -q "FCriticalSection\|FRWLock\|TAtomic\|FScopeLock" "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.h"; then
    log_result "PASS" "Thread safety mechanisms found in ObjectPoolManager"
else
    log_result "ERROR" "No thread safety mechanisms found in ObjectPoolManager"
fi

# Check for memory management patterns
if grep -q "TSharedPtr\|TUniquePtr\|NewObject\|SpawnActor" "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.cpp"; then
    log_result "PASS" "Proper memory management patterns found"
else
    log_result "WARN" "Consider using Unreal's memory management patterns"
fi

echo
echo "3. Performance Validation"
echo "========================="

# Check for performance-critical patterns
if grep -q "FORCEINLINE" "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.h"; then
    log_result "PASS" "FORCEINLINE optimizations found"
else
    log_result "WARN" "Consider FORCEINLINE for performance-critical functions"
fi

# Check for object pooling best practices
if grep -q "Reserve\|SetNum" "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.cpp"; then
    log_result "PASS" "Array pre-allocation patterns found"
else
    log_result "WARN" "Consider pre-allocating arrays for better performance"
fi

# Check for caching mechanisms
if grep -q "TMap\|THashMap" "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.h"; then
    log_result "PASS" "Caching data structures found"
else
    log_result "WARN" "Consider using hash maps for O(1) lookups"
fi

echo
echo "4. Testing Framework Validation"
echo "==============================="

# Check for comprehensive test coverage
TEST_CATEGORIES=("Unit" "Performance" "MemoryStress" "ThreadSafety" "Integration" "Statistics" "Configuration" "Maintenance")
for category in "${TEST_CATEGORIES[@]}"; do
    if grep -q "FAdvancedObjectPool.*${category}Test" "$PROJECT_DIR/Source/FPSGame/Testing/AdvancedObjectPoolTests.h"; then
        log_result "PASS" "${category} tests defined"
    else
        log_result "WARN" "${category} tests not found"
    fi
done

# Check for automation test macros
if grep -q "IMPLEMENT_SIMPLE_AUTOMATION_TEST" "$PROJECT_DIR/Source/FPSGame/Testing/AdvancedObjectPoolTests.cpp"; then
    log_result "PASS" "Automation test framework integration found"
else
    log_result "ERROR" "Automation test framework integration missing"
fi

# Check for test utilities
if grep -q "FPerformanceTimer\|FMemorySnapshot" "$PROJECT_DIR/Source/FPSGame/Testing/AdvancedObjectPoolTests.h"; then
    log_result "PASS" "Performance testing utilities found"
else
    log_result "WARN" "Performance testing utilities missing"
fi

echo
echo "5. Build Configuration Validation"
echo "================================="

# Check build dependencies
BUILD_DEPS=("Engine" "CoreUObject" "UMG" "PhysicsCore" "AutomationController")
for dep in "${BUILD_DEPS[@]}"; do
    if grep -q "\"${dep}\"" "$PROJECT_DIR/Source/FPSGame/FPSGame.Build.cs"; then
        log_result "PASS" "Build dependency '${dep}' found"
    else
        log_result "WARN" "Build dependency '${dep}' not found"
    fi
done

# Check for compilation definitions
if grep -q "WITH_ADVANCED_OBJECT_POOLING" "$PROJECT_DIR/Source/FPSGame/FPSGame.Build.cs"; then
    log_result "PASS" "Advanced object pooling definition found"
else
    log_result "WARN" "Advanced object pooling definition missing"
fi

echo
echo "6. Documentation Validation"
echo "==========================="

# Check for code comments
CPP_FILES=$(find "$PROJECT_DIR/Source" -name "*.cpp" -o -name "*.h" 2>/dev/null)
COMMENT_COUNT=0
for file in $CPP_FILES; do
    if [ -f "$file" ]; then
        COMMENTS=$(grep -c -E "^\s*//|^\s*/\*" "$file" 2>/dev/null)
        if [ -z "$COMMENTS" ]; then
            COMMENTS=0
        fi
        COMMENT_COUNT=$((COMMENT_COUNT + COMMENTS))
    fi
done

if [ $COMMENT_COUNT -gt 100 ]; then
    log_result "PASS" "Adequate code documentation found ($COMMENT_COUNT comments)"
else
    log_result "WARN" "Consider adding more code documentation ($COMMENT_COUNT comments)"
fi

# Check for README files
if [ -f "$PROJECT_DIR/README.md" ]; then
    log_result "PASS" "README documentation exists"
else
    log_result "WARN" "Consider adding README documentation"
fi

echo
echo "7. Security Validation"
echo "======================"

# Check for potential security issues
if grep -r "system\|exec\|eval" "$PROJECT_DIR/Source" --include="*.cpp" --include="*.h" 2>/dev/null; then
    log_result "WARN" "Potential security-sensitive functions found"
else
    log_result "PASS" "No obvious security risks found"
fi

# Check for proper input validation
if grep -q "IsValid\|check\|ensure" "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.cpp"; then
    log_result "PASS" "Input validation patterns found"
else
    log_result "WARN" "Consider adding more input validation"
fi

echo
echo "=========================================="
echo " Validation Summary"
echo "=========================================="
echo -e "Passed: ${GREEN}$VALIDATION_PASSED${NC}"
echo -e "Warnings: ${YELLOW}$VALIDATION_WARNINGS${NC}"
echo -e "Errors: ${RED}$VALIDATION_ERRORS${NC}"
echo

# Overall result
if [ $VALIDATION_ERRORS -eq 0 ]; then
    if [ $VALIDATION_WARNINGS -eq 0 ]; then
        echo -e "${GREEN}✓ VALIDATION PASSED${NC} - All checks passed successfully!"
        exit 0
    else
        echo -e "${YELLOW}⚠ VALIDATION PASSED WITH WARNINGS${NC} - Consider addressing warnings for optimal quality."
        exit 0
    fi
else
    echo -e "${RED}✗ VALIDATION FAILED${NC} - Please fix errors before proceeding."
    exit 1
fi
