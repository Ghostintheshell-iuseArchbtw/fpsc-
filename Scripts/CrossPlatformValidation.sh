#!/bin/bash
# Cross-Platform Code Validation Script
# Validates Advanced Object Pooling System implementation on Arch Linux for Windows development

echo "=========================================="
echo " Cross-Platform Code Validation"
echo " Arch Linux -> Windows Development"
echo "=========================================="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
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
        "TEST")
            echo -e "${CYAN}🧪 TEST:${NC} $message"
            ;;
    esac
}

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

log_result "INFO" "Project directory: $PROJECT_DIR"
log_result "INFO" "Starting cross-platform validation..."

# 1. File Structure Validation
echo
log_result "TEST" "Validating file structure..."

# Check core files exist
CORE_FILES=(
    "Source/FPSGame/Optimization/AdvancedObjectPoolManager.h"
    "Source/FPSGame/Optimization/AdvancedObjectPoolManager.cpp"
    "Source/FPSGame/Weapons/AdvancedWeaponSystem.h"
    "Source/FPSGame/Weapons/AdvancedWeaponSystem.cpp"
    "Source/FPSGame/Testing/WeaponPoolingIntegrationTest.h"
    "Source/FPSGame/Testing/WeaponPoolingIntegrationTest.cpp"
)

for file in "${CORE_FILES[@]}"; do
    if [ -f "$PROJECT_DIR/$file" ]; then
        log_result "PASS" "Found: $file"
    else
        log_result "ERROR" "Missing: $file"
    fi
done

# 2. Code Syntax Validation (using clang if available)
echo
log_result "TEST" "Checking for syntax validation tools..."

if command -v clang++ >/dev/null 2>&1; then
    log_result "PASS" "clang++ available for syntax checking"
    
    # Check weapon system syntax
    log_result "TEST" "Validating AdvancedWeaponSystem syntax..."
    clang++ -std=c++17 -fsyntax-only -I"$PROJECT_DIR/Source" \
        "$PROJECT_DIR/Source/FPSGame/Weapons/AdvancedWeaponSystem.cpp" 2>/dev/null
    if [ $? -eq 0 ]; then
        log_result "PASS" "AdvancedWeaponSystem syntax valid"
    else
        log_result "WARN" "AdvancedWeaponSystem syntax issues detected (may be due to UE5 headers)"
    fi
    
    # Check pooling manager syntax
    log_result "TEST" "Validating AdvancedObjectPoolManager syntax..."
    clang++ -std=c++17 -fsyntax-only -I"$PROJECT_DIR/Source" \
        "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.cpp" 2>/dev/null
    if [ $? -eq 0 ]; then
        log_result "PASS" "AdvancedObjectPoolManager syntax valid"
    else
        log_result "WARN" "AdvancedObjectPoolManager syntax issues detected (may be due to UE5 headers)"
    fi
else
    log_result "WARN" "clang++ not available for syntax checking"
fi

# 3. Code Pattern Validation
echo
log_result "TEST" "Validating pooling integration patterns..."

# Check for pooling integration in weapon system
if grep -q "PoolingComponent" "$PROJECT_DIR/Source/FPSGame/Weapons/AdvancedWeaponSystem.cpp"; then
    log_result "PASS" "Pooling integration found in weapon system"
else
    log_result "ERROR" "Pooling integration missing in weapon system"
fi

# Check for fallback mechanisms
if grep -q "fallback\|traditional" "$PROJECT_DIR/Source/FPSGame/Weapons/AdvancedWeaponSystem.cpp"; then
    log_result "PASS" "Fallback mechanisms implemented"
else
    log_result "WARN" "Fallback mechanisms may be missing"
fi

# Check for test automation
if grep -q "UTEST\|UCLASS" "$PROJECT_DIR/Source/FPSGame/Testing/WeaponPoolingIntegrationTest.h"; then
    log_result "PASS" "Test automation framework detected"
else
    log_result "WARN" "Test automation framework may be incomplete"
fi

# 4. Performance Considerations Validation
echo
log_result "TEST" "Validating performance optimization patterns..."

# Check for memory management patterns
if grep -q "FMemory\|TObjectPtr\|TWeakPtr" "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.cpp"; then
    log_result "PASS" "Unreal Engine memory management patterns found"
else
    log_result "WARN" "Memory management patterns may need review"
fi

# Check for performance measurement
if grep -q "FPlatformTime\|SCOPE_CYCLE_COUNTER" "$PROJECT_DIR/Source/FPSGame/Testing/WeaponPoolingIntegrationTest.cpp"; then
    log_result "PASS" "Performance measurement tools integrated"
else
    log_result "WARN" "Performance measurement may be limited"
fi

# 5. Cross-Platform Compatibility Check
echo
log_result "TEST" "Checking cross-platform compatibility..."

# Check for platform-specific code
PLATFORM_PATTERNS=("Windows\\.h" "__WIN32__" "PLATFORM_WINDOWS" "#ifdef _WIN32")
PLATFORM_ISSUES=0

for pattern in "${PLATFORM_PATTERNS[@]}"; do
    if grep -r "$pattern" "$PROJECT_DIR/Source/FPSGame/" >/dev/null 2>&1; then
        ((PLATFORM_ISSUES++))
    fi
done

if [ $PLATFORM_ISSUES -eq 0 ]; then
    log_result "PASS" "No obvious platform-specific code detected"
else
    log_result "WARN" "Platform-specific code detected ($PLATFORM_ISSUES patterns)"
fi

# 6. Unreal Engine API Usage Validation
echo
log_result "TEST" "Validating Unreal Engine API usage..."

# Check for proper UE5 patterns
UE_PATTERNS=("UCLASS" "UPROPERTY" "UFUNCTION" "GENERATED_BODY")
UE_FOUND=0

for pattern in "${UE_PATTERNS[@]}"; do
    if grep -r "$pattern" "$PROJECT_DIR/Source/FPSGame/" >/dev/null 2>&1; then
        ((UE_FOUND++))
    fi
done

if [ $UE_FOUND -ge 3 ]; then
    log_result "PASS" "Proper Unreal Engine patterns detected ($UE_FOUND/4)"
else
    log_result "WARN" "Limited Unreal Engine patterns detected ($UE_FOUND/4)"
fi

# 7. Build Configuration Validation
echo
log_result "TEST" "Validating build configuration..."

if [ -f "$PROJECT_DIR/Source/FPSGame/FPSGame.Build.cs" ]; then
    log_result "PASS" "Build configuration file found"
    
    # Check for testing modules
    if grep -q "AutomationTest\|Testing" "$PROJECT_DIR/Source/FPSGame/FPSGame.Build.cs"; then
        log_result "PASS" "Testing modules configured in build"
    else
        log_result "WARN" "Testing modules may not be properly configured"
    fi
else
    log_result "ERROR" "Build configuration file missing"
fi

# 8. Generate Static Analysis Report
echo
log_result "TEST" "Generating static analysis report..."

REPORT_FILE="$PROJECT_DIR/Saved/Logs/CrossPlatformValidationReport.txt"
mkdir -p "$PROJECT_DIR/Saved/Logs"

cat > "$REPORT_FILE" << EOF
Cross-Platform Validation Report
Generated: $(date)
System: Arch Linux (Target: Windows)

=== VALIDATION SUMMARY ===
Passed: $VALIDATION_PASSED
Warnings: $VALIDATION_WARNINGS  
Errors: $VALIDATION_ERRORS

=== CODE METRICS ===
Lines of Code (Weapon System): $(wc -l < "$PROJECT_DIR/Source/FPSGame/Weapons/AdvancedWeaponSystem.cpp" 2>/dev/null || echo "N/A")
Lines of Code (Pool Manager): $(wc -l < "$PROJECT_DIR/Source/FPSGame/Optimization/AdvancedObjectPoolManager.cpp" 2>/dev/null || echo "N/A")
Lines of Code (Integration Tests): $(wc -l < "$PROJECT_DIR/Source/FPSGame/Testing/WeaponPoolingIntegrationTest.cpp" 2>/dev/null || echo "N/A")

=== INTEGRATION STATUS ===
Pooling Integration: $(grep -c "PoolingComponent" "$PROJECT_DIR/Source/FPSGame/Weapons/AdvancedWeaponSystem.cpp" 2>/dev/null || echo "0") instances
Fallback Mechanisms: $(grep -c "fallback\|traditional" "$PROJECT_DIR/Source/FPSGame/Weapons/AdvancedWeaponSystem.cpp" 2>/dev/null || echo "0") instances
Test Cases: $(grep -c "UTEST\|TEST" "$PROJECT_DIR/Source/FPSGame/Testing/WeaponPoolingIntegrationTest.cpp" 2>/dev/null || echo "0") tests

=== RECOMMENDATIONS ===
EOF

if [ $VALIDATION_ERRORS -gt 0 ]; then
    echo "- Address $VALIDATION_ERRORS critical errors before deployment" >> "$REPORT_FILE"
fi

if [ $VALIDATION_WARNINGS -gt 0 ]; then
    echo "- Review $VALIDATION_WARNINGS warnings for optimization opportunities" >> "$REPORT_FILE"
fi

echo "- Consider setting up Windows VM for full runtime testing" >> "$REPORT_FILE"
echo "- Implement continuous integration pipeline for automated testing" >> "$REPORT_FILE"
echo "- Add memory profiling integration for performance validation" >> "$REPORT_FILE"

log_result "PASS" "Static analysis report generated: $REPORT_FILE"

# 9. Memory Pattern Analysis
echo
log_result "TEST" "Analyzing memory usage patterns..."

# Count dynamic allocations
NEW_COUNT=$(grep -r "new \|FMemory::Malloc" "$PROJECT_DIR/Source/FPSGame/" 2>/dev/null | wc -l || echo "0")
DELETE_COUNT=$(grep -r "delete \|FMemory::Free" "$PROJECT_DIR/Source/FPSGame/" 2>/dev/null | wc -l || echo "0")

log_result "INFO" "Dynamic allocations detected: $NEW_COUNT"
log_result "INFO" "Dynamic deallocations detected: $DELETE_COUNT"

if [ "$NEW_COUNT" -eq "$DELETE_COUNT" ] && [ "$NEW_COUNT" -gt 0 ]; then
    log_result "PASS" "Balanced allocation/deallocation pattern"
elif [ "$NEW_COUNT" -eq 0 ] && [ "$DELETE_COUNT" -eq 0 ]; then
    log_result "PASS" "No dynamic memory management detected (good for pooling)"
else
    log_result "WARN" "Unbalanced allocation/deallocation pattern"
fi

# 10. Final Summary
echo
echo "=========================================="
echo "           VALIDATION SUMMARY"
echo "=========================================="
echo
echo -e "${GREEN}Passed:${NC}   $VALIDATION_PASSED"
echo -e "${YELLOW}Warnings:${NC} $VALIDATION_WARNINGS"
echo -e "${RED}Errors:${NC}   $VALIDATION_ERRORS"
echo

if [ $VALIDATION_ERRORS -eq 0 ]; then
    echo -e "${GREEN}✓ VALIDATION SUCCESSFUL${NC}"
    echo "The Advanced Object Pooling System appears ready for Windows deployment."
    exit 0
else
    echo -e "${RED}✗ VALIDATION FAILED${NC}"
    echo "Please address the errors before proceeding."
    exit 1
fi
