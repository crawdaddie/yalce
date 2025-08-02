#!/bin/bash

set -e  # Exit on any error

# Detect OS
detect_os() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif command -v pacman > /dev/null 2>&1; then
        echo "arch"
    elif command -v apt > /dev/null 2>&1; then
        echo "ubuntu"
    else
        echo "unknown"
    fi
}

OS=$(detect_os)
echo "Detected OS: $OS"

# Define package manager and standard paths based on detected OS
case $OS in
    macos)
        PKG_MANAGER="brew"
        INCLUDE_PATHS=("/usr/include" "/usr/local/include" "/opt/homebrew/include")
        LIBRARY_PATHS=("/usr/lib" "/usr/local/lib" "/opt/homebrew/lib")
        DEFAULT_CPATH="/opt/homebrew/include"
        DEFAULT_LIBRARY_PATH="/opt/homebrew/lib"
        ;;
    arch)
        PKG_MANAGER="pacman"
        INCLUDE_PATHS=("/usr/include")
        LIBRARY_PATHS=("/usr/lib")
        DEFAULT_CPATH="/usr/include"
        DEFAULT_LIBRARY_PATH="/usr/lib"
        ;;
    ubuntu)
        PKG_MANAGER="apt"
        INCLUDE_PATHS=("/usr/include")
        LIBRARY_PATHS=("/usr/lib" "/usr/lib/x86_64-linux-gnu")
        DEFAULT_CPATH="/usr/include"
        DEFAULT_LIBRARY_PATH="/usr/lib"
        ;;
    *)
        echo "Unsupported OS detected. Exiting."
        exit 1
        ;;
esac

# Package name mappings for different OSes
declare -A arch_pkgs=(
    ["llvm"]="llvm"
    ["sdl2"]="sdl2"
    ["sdl2_ttf"]="sdl2_ttf"
    ["sdl2_gfx"]="sdl2_gfx"
    ["readline"]="readline"
    ["libsoundio"]="libsoundio"
    ["libsndfile"]="libsndfile"
    ["fftw"]="fftw"
)

declare -A ubuntu_pkgs=(
    ["llvm"]="llvm-dev"
    ["sdl2"]="libsdl2-dev"
    ["sdl2_ttf"]="libsdl2-ttf-dev"
    ["sdl2_gfx"]="libsdl2-gfx-dev"
    ["readline"]="libreadline-dev"
    ["libsoundio"]="libsoundio-dev"
    ["libsndfile"]="libsndfile1-dev"
    ["fftw"]="libfftw3-dev"
)

# Function to check if a library exists
check_library() {
    local lib_name=$1
    
    # Check standard include paths
    for base_path in "${INCLUDE_PATHS[@]}"; do
        local path="$base_path/$lib_name"
        if [ -d "$path" ]; then
            echo "$path"
            return 0
        fi
    done
    
    # Special handling for different libraries
    case $lib_name in
        "llvm")
            case $OS in
                "ubuntu")
                    # Check for LLVM in versioned directories
                    for ver in {18..10}; do
                        if [ -d "/usr/lib/llvm-$ver" ]; then
                            echo "/usr/lib/llvm-$ver"
                            return 0
                        fi
                    done
                    # Check for llvm-config
                    if command -v llvm-config > /dev/null 2>&1; then
                        local llvm_prefix=$(llvm-config --prefix)
                        if [ -n "$llvm_prefix" ] && [ -d "$llvm_prefix" ]; then
                            echo "$llvm_prefix"
                            return 0
                        fi
                    fi
                    ;;
                "macos")
                    if command -v brew > /dev/null 2>&1; then
                        local brew_prefix=$(brew --prefix llvm 2>/dev/null)
                        if [ -n "$brew_prefix" ] && [ -d "$brew_prefix" ]; then
                            echo "$brew_prefix"
                            return 0
                        fi
                    fi
                    ;;
                "arch")
                    if [ -d "/usr/include/llvm" ]; then
                        echo "/usr"
                        return 0
                    fi
                    ;;
            esac
            ;;
        "sdl2"|"SDL2")
            # Check for SDL2 specifically
            for base_path in "${INCLUDE_PATHS[@]}"; do
                if [ -f "$base_path/SDL2/SDL.h" ]; then
                    echo "$base_path"
                    return 0
                fi
            done
            ;;
        *)
            # For other libraries, check if headers exist
            for base_path in "${INCLUDE_PATHS[@]}"; do
                # Try different naming conventions
                for header_path in "$base_path/$lib_name" "$base_path/${lib_name/-/_}" "$base_path/${lib_name/lib/}"; do
                    if [ -d "$header_path" ]; then
                        echo "$base_path"
                        return 0
                    fi
                done
            done
            ;;
    esac
    
    return 1
}

# Function to install a package based on OS
install_package() {
    local pkg_name=$1
    local success=false
    
    echo "Installing $pkg_name..."
    
    case $OS in
        macos)
            if command -v brew > /dev/null 2>&1; then
                if brew install "$pkg_name"; then
                    success=true
                fi
            else
                echo "Error: Homebrew not found. Please install Homebrew first."
                return 1
            fi
            ;;
        arch)
            if [[ -n "${arch_pkgs[$pkg_name]}" ]]; then
                if sudo pacman -S --noconfirm "${arch_pkgs[$pkg_name]}"; then
                    success=true
                fi
            else
                echo "Warning: No mapping for $pkg_name on Arch Linux"
                return 1
            fi
            ;;
        ubuntu)
            # Update package list first
            sudo apt-get update -qq
            if [[ -n "${ubuntu_pkgs[$pkg_name]}" ]]; then
                if sudo apt-get install -y "${ubuntu_pkgs[$pkg_name]}"; then
                    success=true
                fi
            else
                echo "Warning: No mapping for $pkg_name on Ubuntu"
                return 1
            fi
            ;;
    esac
    
    if [ "$success" = true ]; then
        echo "Successfully installed $pkg_name"
        return 0
    else
        echo "Failed to install $pkg_name"
        return 1
    fi
}

# Function to get library path after installation
get_lib_path() {
    local lib_name=$1
    
    case $OS in
        macos)
            if command -v brew > /dev/null 2>&1; then
                local brew_path=$(brew --prefix "$lib_name" 2>/dev/null)
                if [ -n "$brew_path" ] && [ -d "$brew_path" ]; then
                    echo "$brew_path"
                    return 0
                fi
            fi
            # Fallback to default paths
            for path in "/opt/homebrew" "/usr/local"; do
                if [ -d "$path" ]; then
                    echo "$path"
                    return 0
                fi
            done
            ;;
        arch|ubuntu)
            case $lib_name in
                "llvm")
                    if [[ $OS == "ubuntu" ]]; then
                        # Find the highest version LLVM
                        for ver in {18..10}; do
                            if [ -d "/usr/lib/llvm-$ver" ]; then
                                echo "/usr/lib/llvm-$ver"
                                return 0
                            fi
                        done
                    fi
                    echo "/usr"
                    ;;
                *)
                    echo "/usr"
                    ;;
            esac
            ;;
    esac
}

# Create backup of existing .env if it exists
if [ -f ".env" ]; then
    cp .env .env.backup
    echo "Backed up existing .env to .env.backup"
fi

# Initialize .env with default paths
cat > .env << EOF
# Auto-generated environment configuration
export CPATH=$DEFAULT_CPATH
export LIBRARY_PATH=$DEFAULT_LIBRARY_PATH
EOF

# Library definitions: {package_name};{env_var}
libs=(
    "llvm;LLVM_PATH"
    "sdl2;SDL2_PATH"
    "sdl2_ttf;SDL2_TTF_PATH"
    "sdl2_gfx;SDL2_GFX_PATH"
    "readline;READLINE_PREFIX"
    "libsoundio;LIBSOUNDIO_PATH"
    "libsndfile;LIBSNDFILE_PATH"
    "fftw;LIBFFTW3_PATH"
)

echo "Checking and installing required libraries..."

# Install and configure libraries
for lib in "${libs[@]}"; do
    IFS=";" read -r lib_name env_var <<< "$lib"
    
    # Remove version suffix for directory checking
    lib_dir_name=${lib_name%%@*}
    
    echo "Checking for $lib_name..."
    
    if path=$(check_library "$lib_dir_name"); then
        echo "✓ $lib_name found at: $path"
    else
        echo "✗ $lib_name not found, attempting to install..."
        if install_package "$lib_name"; then
            # Re-check after installation
            if path=$(check_library "$lib_dir_name"); then
                echo "✓ $lib_name installed and found at: $path"
            else
                path=$(get_lib_path "$lib_name")
                echo "✓ $lib_name installed, using default path: $path"
            fi
        else
            echo "✗ Failed to install $lib_name"
            path=$(get_lib_path "$lib_name")
            echo "Using default path: $path"
        fi
    fi
    
    echo "export $env_var=$path" >> .env
done

echo ""
echo "Setup completed for $OS!"
echo "Environment variables have been written to .env"
echo "To use these variables, run: source .env"
echo ""
echo "Installed library paths:"
cat .env | grep -E '^export.*_PATH=' | sed 's/export /  /'
