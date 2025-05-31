#!/bin/bash
# Arch Linux specific build configuration for Advanced FPS Game
# This script handles Unreal Engine installation and setup on Arch Linux

echo "=========================================="
echo " Arch Linux Build Configuration"
echo "=========================================="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Arch Linux specific paths and configurations
ARCH_UE5_PATH="/opt/unreal-engine"
AUR_UE5_PATH="/usr/share/unreal-engine"
USER_UE5_PATH="$HOME/UnrealEngine"

# Function to detect Unreal Engine installation
detect_unreal_engine() {
    echo -e "${BLUE}ℹ INFO:${NC} Detecting Unreal Engine installation..."
    
    # Check common Arch Linux installation paths
    if [ -d "$ARCH_UE5_PATH" ]; then
        export UE5_ROOT="$ARCH_UE5_PATH"
        echo -e "${GREEN}✓ FOUND:${NC} Unreal Engine at $ARCH_UE5_PATH"
        return 0
    elif [ -d "$AUR_UE5_PATH" ]; then
        export UE5_ROOT="$AUR_UE5_PATH"
        echo -e "${GREEN}✓ FOUND:${NC} Unreal Engine at $AUR_UE5_PATH"
        return 0
    elif [ -d "$USER_UE5_PATH" ]; then
        export UE5_ROOT="$USER_UE5_PATH"
        echo -e "${GREEN}✓ FOUND:${NC} Unreal Engine at $USER_UE5_PATH"
        return 0
    else
        echo -e "${YELLOW}⚠ WARN:${NC} Unreal Engine not found. Checking if available via AUR..."
        return 1
    fi
}

# Function to install dependencies via pacman
install_dependencies() {
    echo -e "${BLUE}ℹ INFO:${NC} Installing build dependencies..."
    
    # Essential build tools
    ESSENTIAL_DEPS=(
        "base-devel"
        "clang"
        "llvm"
        "cmake"
        "ninja"
        "git"
        "mono"
        "dotnet-runtime"
        "dotnet-sdk"
    )
    
    # Graphics and multimedia dependencies
    GRAPHICS_DEPS=(
        "vulkan-devel"
        "mesa"
        "lib32-mesa"
        "nvidia-utils" # Optional, for NVIDIA users
        "libx11"
        "libxrandr"
        "libxi"
        "libxinerama"
        "libxcursor"
    )
    
    # Audio dependencies
    AUDIO_DEPS=(
        "alsa-lib"
        "pulseaudio"
        "pipewire"
        "jack2"
    )
    
    echo "Installing essential dependencies..."
    for dep in "${ESSENTIAL_DEPS[@]}"; do
        if ! pacman -Qi "$dep" >/dev/null 2>&1; then
            echo "Installing $dep..."
            sudo pacman -S --noconfirm "$dep" || echo -e "${YELLOW}⚠ WARN:${NC} Failed to install $dep"
        else
            echo -e "${GREEN}✓${NC} $dep already installed"
        fi
    done
    
    echo "Installing graphics dependencies..."
    for dep in "${GRAPHICS_DEPS[@]}"; do
        if ! pacman -Qi "$dep" >/dev/null 2>&1; then
            echo "Installing $dep..."
            sudo pacman -S --noconfirm "$dep" 2>/dev/null || echo -e "${YELLOW}⚠ WARN:${NC} $dep not available or failed to install"
        fi
    done
    
    echo "Installing audio dependencies..."
    for dep in "${AUDIO_DEPS[@]}"; do
        if ! pacman -Qi "$dep" >/dev/null 2>&1; then
            echo "Installing $dep..."
            sudo pacman -S --noconfirm "$dep" 2>/dev/null || echo -e "${YELLOW}⚠ WARN:${NC} $dep not available or failed to install"
        fi
    done
    
    echo -e "${GREEN}✓${NC} Dependencies installation completed"
}

# Function to setup Unreal Engine from source (if not installed)
setup_unreal_from_source() {
    echo -e "${BLUE}ℹ INFO:${NC} Setting up Unreal Engine from source..."
    
    if [ ! -d "$USER_UE5_PATH" ]; then
        echo "Cloning Unreal Engine repository..."
        git clone https://github.com/EpicGames/UnrealEngine.git "$USER_UE5_PATH"
        cd "$USER_UE5_PATH"
        
        echo "Setting up Unreal Engine..."
        ./Setup.sh
        ./GenerateProjectFiles.sh
        
        echo "Building Unreal Engine (this may take several hours)..."
        make
        
        export UE5_ROOT="$USER_UE5_PATH"
        echo -e "${GREEN}✓${NC} Unreal Engine built from source"
    else
        echo -e "${GREEN}✓${NC} Unreal Engine source already exists"
        export UE5_ROOT="$USER_UE5_PATH"
    fi
}

# Function to configure build environment
configure_build_environment() {
    echo -e "${BLUE}ℹ INFO:${NC} Configuring build environment..."
    
    # Set environment variables
    export CC=clang
    export CXX=clang++
    export CXXFLAGS="-std=c++17 -fPIC"
    export LDFLAGS="-fuse-ld=lld"
    
    # Arch Linux specific library paths
    export LD_LIBRARY_PATH="/usr/lib:/usr/lib32:$LD_LIBRARY_PATH"
    
    # Set Vulkan SDK path if available
    if [ -d "/usr/include/vulkan" ]; then
        export VULKAN_SDK="/usr"
        echo -e "${GREEN}✓${NC} Vulkan SDK found"
    fi
    
    # Configure mono if available
    if command -v mono >/dev/null 2>&1; then
        export MONO_PATH="/usr/lib/mono"
        echo -e "${GREEN}✓${NC} Mono configured"
    fi
    
    echo -e "${GREEN}✓${NC} Build environment configured"
}

# Function to build the project
build_project() {
    echo -e "${BLUE}ℹ INFO:${NC} Building Advanced FPS Game project..."
    
    local PROJECT_DIR="$(dirname "$(dirname "$(realpath "$0")")")"
    cd "$PROJECT_DIR"
    
    if [ -z "$UE5_ROOT" ]; then
        echo -e "${RED}✗ ERROR:${NC} UE5_ROOT not set. Cannot build project."
        return 1
    fi
    
    local UBT_PATH="$UE5_ROOT/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool"
    
    if [ ! -f "$UBT_PATH" ]; then
        # Try alternative path for built-from-source UE
        UBT_PATH="$UE5_ROOT/Engine/Binaries/Linux/UnrealBuildTool"
    fi
    
    if [ ! -f "$UBT_PATH" ]; then
        echo -e "${RED}✗ ERROR:${NC} UnrealBuildTool not found"
        return 1
    fi
    
    echo "Generating project files..."
    "$UBT_PATH" -projectfiles -project="$PROJECT_DIR/FPSGame.uproject" -game -rocket -progress
    
    echo "Building editor target..."
    "$UBT_PATH" FPSGameEditor Linux Development -project="$PROJECT_DIR/FPSGame.uproject" -rocket -progress
    
    echo "Building game target..."
    "$UBT_PATH" FPSGame Linux Development -project="$PROJECT_DIR/FPSGame.uproject" -rocket -progress
    
    echo -e "${GREEN}✓${NC} Project build completed"
}

# Function to run performance benchmarks
run_performance_benchmarks() {
    echo -e "${BLUE}ℹ INFO:${NC} Running performance benchmarks..."
    
    # Create benchmark results directory
    mkdir -p "Saved/Benchmarks"
    
    # CPU benchmark
    echo "Running CPU benchmark..."
    if command -v sysbench >/dev/null 2>&1; then
        sysbench cpu --threads=$(nproc) run > "Saved/Benchmarks/cpu_benchmark.txt"
    else
        echo "CPU cores: $(nproc)" > "Saved/Benchmarks/cpu_benchmark.txt"
        echo "CPU info:" >> "Saved/Benchmarks/cpu_benchmark.txt"
        lscpu >> "Saved/Benchmarks/cpu_benchmark.txt"
    fi
    
    # Memory benchmark
    echo "Running memory benchmark..."
    free -h > "Saved/Benchmarks/memory_benchmark.txt"
    
    # GPU benchmark (if available)
    echo "Checking GPU capabilities..."
    if command -v glxinfo >/dev/null 2>&1; then
        glxinfo | grep -E "(OpenGL|Direct|Vendor)" > "Saved/Benchmarks/gpu_info.txt"
    fi
    
    if command -v vulkaninfo >/dev/null 2>&1; then
        vulkaninfo --summary > "Saved/Benchmarks/vulkan_info.txt" 2>/dev/null
    fi
    
    echo -e "${GREEN}✓${NC} Performance benchmarks completed"
}

# Main execution
main() {
    echo "Starting Arch Linux build configuration..."
    
    # Install dependencies
    install_dependencies
    
    # Detect or setup Unreal Engine
    if ! detect_unreal_engine; then
        echo -e "${YELLOW}⚠ WARN:${NC} Unreal Engine not found. Attempting to set up from source..."
        echo "This will download and build Unreal Engine, which may take several hours."
        echo "Press Ctrl+C to cancel, or Enter to continue..."
        read -r
        
        setup_unreal_from_source
    fi
    
    # Configure build environment
    configure_build_environment
    
    # Build the project
    if [ "$1" != "setup-only" ]; then
        build_project
        
        # Run benchmarks if requested
        if [ "$2" == "benchmark" ]; then
            run_performance_benchmarks
        fi
    fi
    
    echo
    echo -e "${GREEN}✓ SUCCESS:${NC} Arch Linux build configuration completed!"
    echo
    echo "Environment variables set:"
    echo "  UE5_ROOT=$UE5_ROOT"
    echo "  CC=$CC"
    echo "  CXX=$CXX"
    echo
    echo "To build manually, run:"
    echo "  cd $(dirname "$(dirname "$(realpath "$0")")")"
    echo "  \$UE5_ROOT/Engine/Binaries/Linux/UnrealBuildTool FPSGame Linux Development -project=FPSGame.uproject"
}

# Parse command line arguments
case "$1" in
    "help"|"-h"|"--help")
        echo "Usage: $0 [setup-only] [benchmark]"
        echo "  setup-only  - Only install dependencies and setup environment"
        echo "  benchmark   - Run performance benchmarks after build"
        echo "  help        - Show this help message"
        exit 0
        ;;
    *)
        main "$@"
        ;;
esac
