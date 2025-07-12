#!/bin/bash

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
    ["llvm@16"]="llvm"
    ["sdl2"]="sdl2"
    ["sdl2_ttf"]="sdl2_ttf"
    ["sdl2_gfx"]="sdl2_gfx"
    ["readline"]="readline"
    ["libsoundio"]="libsoundio"
    ["libsndfile"]="libsndfile"
    ["fftw"]="fftw"
    ["openblas"]="openblas"
    ["libomp"]="openmp"
)

declare -A ubuntu_pkgs=(
    ["llvm@16"]="llvm-16"
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
    
    for base_path in "${INCLUDE_PATHS[@]}"; do
        local path="$base_path/$lib_name"
        [ -d "$path" ] && echo "$path" && return
    done
    
    # Additional check for Ubuntu's LLVM-specific path
    if [[ $OS == "ubuntu" && $lib_name == "llvm"* ]]; then
        for ver in {10..16}; do
            local path="/usr/lib/llvm-$ver"
            [ -d "$path" ] && echo "$path" && return
        done
    fi
    
    echo ""
}

# Function to check repository
check_repo() {
    [ -d "$1/.git" ] && echo "$1" || echo ""
}

# Function to install a package based on OS
install_package() {
    local brew_pkg=$1
    
    case $OS in
        macos)
            brew install "$brew_pkg"
            ;;
        arch)
            if [[ -n "${arch_pkgs[$brew_pkg]}" ]]; then
                sudo pacman -S --noconfirm "${arch_pkgs[$brew_pkg]}"
            else
                echo "Warning: No mapping for $brew_pkg on Arch Linux"
            fi
            ;;
        ubuntu)
            if [[ -n "${ubuntu_pkgs[$brew_pkg]}" ]]; then
                sudo apt-get install -y "${ubuntu_pkgs[$brew_pkg]}"
            else
                echo "Warning: No mapping for $brew_pkg on Ubuntu"
            fi
            ;;
    esac
}

# Function to get library path after installation
get_lib_path() {
    local lib_name=$1
    
    case $OS in
        macos)
            brew --prefix "$lib_name"
            ;;
        arch|ubuntu)
            # For Linux, most libraries are installed to standard paths
            if [[ $lib_name == "llvm"* ]]; then
                if [[ $OS == "ubuntu" ]]; then
                    echo "/usr/lib/llvm-16"
                else
                    echo "/usr"
                fi
            else
                echo "/usr"
            fi
            ;;
    esac
}

# Initialize .env with default paths
cat > .env << EOF
export CPATH=$DEFAULT_CPATH
export LIBRARY_PATH=$DEFAULT_LIBRARY_PATH
EOF

# Library definitions: {brew_package};{env_var}
libs=(
    "llvm@16;LLVM_PATH"
    "sdl2;SDL2_PATH"
    "sdl2_ttf;SDL2_TTF_PATH"
    "sdl2_gfx;SDL2_GFX_PATH"
    "readline;READLINE_PREFIX"
    "libsoundio;LIBSOUNDIO_PATH"
    "libsndfile;LIBSNDFILE_PATH"
    "fftw;LIBFFTW3_PATH"
    "openblas;OPENBLAS_PATH"
    "libomp;OPENMP_PATH"
)

# Install and configure libraries
for lib in "${libs[@]}"; do
    IFS=";" read -r lib_name env_var <<< "$lib"
    
    lib_dir_name=${lib_name%%@*}  # Remove version suffix for directory checking
    
    if path=$(check_library "$lib_dir_name"); then
        echo "$lib_name Found at: $path"
    else
        echo "Installing $lib_name..."
        install_package "$lib_name"
        path=$(get_lib_path "$lib_name")
    fi
    echo "export $env_var=$path" >> .env
done

# Setup CLAP submodule
if [ -z "$(check_repo "$PWD/libs/clap")" ]; then
    git submodule update --init --recursive
fi
echo "export CLAP_PATH=$PWD/libs/clap" >> .env

echo "Setup completed successfully for $OS"
