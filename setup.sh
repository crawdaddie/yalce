#!/bin/bash

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

OS=${YLC_OS:-$(detect_os)}
echo "Detected OS: $OS"

case $OS in
    macos)
        PKG_MANAGER="brew"
        BREW_PREFIX=$(brew --prefix)
        INCLUDE_PATHS=("/usr/include" "/usr/local/include" "$BREW_PREFIX/include")
        LIBRARY_PATHS=("/usr/lib" "/usr/local/lib" "$BREW_PREFIX/lib")
        DEFAULT_CPATH="$BREW_PREFIX/include"
        DEFAULT_LIBRARY_PATH="$BREW_PREFIX/lib"
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

arch_package() {
    case $1 in
        llvm|sdl2|sdl2_ttf|sdl2_gfx|sdl2_image|readline|libsoundio|libsndfile|fftw|glew|libxml2)
            echo "$1"
            ;;
        glfw)
            echo "glfw-x11"
            ;;
    esac
}

ubuntu_package() {
    case $1 in
        llvm) echo "llvm" ;;
        sdl2) echo "libsdl2-dev" ;;
        sdl2_ttf) echo "libsdl2-ttf-dev" ;;
        sdl2_gfx) echo "libsdl2-gfx-dev" ;;
        readline) echo "libreadline-dev" ;;
        libsoundio) echo "libsoundio-dev" ;;
        libsndfile) echo "libsndfile1-dev" ;;
        fftw) echo "libfftw3-dev" ;;
    esac
}

check_library() {
    local lib_name=$1

    # For Arch, check if package is installed using pacman
    if [[ $OS == "arch" ]]; then
        local pkg_name
        pkg_name=$(arch_package "$lib_name")
        if [[ -n "$pkg_name" ]] && pacman -Q "$pkg_name" > /dev/null 2>&1; then
            # Package is installed, return standard path
            echo "/usr"
            return
        fi
        echo ""
        return
    fi

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

check_repo() {
    [ -d "$1/.git" ] && echo "$1" || echo ""
}

install_package() {
    local brew_pkg=$1

    case $OS in
        macos)
            brew install "$brew_pkg"
            ;;
        arch)
            local pkg
            pkg=$(arch_package "$brew_pkg")
            if [[ -n "$pkg" ]]; then
                # Try pacman first
                if sudo pacman -S --noconfirm "$pkg" 2>/dev/null; then
                    echo "$pkg installed successfully"
                else
                    # If pacman fails, try AUR helper
                    echo "$pkg not in official repos, attempting AUR installation..."
                    if command -v yay > /dev/null 2>&1; then
                        yay -S --noconfirm "$pkg"
                    elif command -v paru > /dev/null 2>&1; then
                        paru -S --noconfirm "$pkg"
                    else
                        echo "Warning: $pkg not found in official repos and no AUR helper (yay/paru) installed"
                        echo "Please install $pkg manually from AUR"
                    fi
                fi
            else
                echo "Warning: No mapping for $brew_pkg on Arch Linux"
            fi
            ;;
        ubuntu)
            local pkg
            pkg=$(ubuntu_package "$brew_pkg")
            if [[ -n "$pkg" ]]; then
                sudo apt-get install -y "$pkg"
            else
                echo "Warning: No mapping for $brew_pkg on Ubuntu"
            fi
            ;;
    esac
}

get_lib_path() {
    local lib_name=$1

    case $OS in
        macos)
            brew --prefix "$lib_name"
            ;;
        arch|ubuntu)
            # For Linux, most libraries are installed to standard paths
            echo "/usr"
            ;;
    esac
}

package_name() {
    if [[ $OS == "macos" && $1 == "llvm" ]]; then
        echo "llvm@21"
        return
    fi
    echo "$1"
}

# Install build tools for Arch Linux
if [[ $OS == "arch" ]]; then
    echo "Checking build tools..."
    for tool in base-devel bison flex git clang; do
        if ! pacman -Q "$tool" > /dev/null 2>&1; then
            echo "Installing $tool..."
            sudo pacman -S --noconfirm "$tool"
        else
            echo "$tool already installed"
        fi
    done

    # Install additional dependencies for GUI and engine
    echo "Checking additional dependencies..."
    additional_deps=("glew" "glfw-x11" "libxml2" "sdl2_image")
    for dep in "${additional_deps[@]}"; do
        if ! pacman -Q "$dep" > /dev/null 2>&1; then
            echo "Installing $dep..."
            sudo pacman -S --noconfirm "$dep"
        else
            echo "$dep already installed"
        fi
    done
fi

cat > .env << EOF
export CPATH=$DEFAULT_CPATH
export LIBRARY_PATH=$DEFAULT_LIBRARY_PATH
EOF

libs=(
    "llvm;LLVM_PATH"
    "sdl2;SDL2_PATH"
    "sdl2_ttf;SDL2_TTF_PATH"
    "sdl2_gfx;SDL2_GFX_PATH"
    "sdl2_image;SDL2_IMAGE_PATH"
    "readline;READLINE_PREFIX"
    "libsoundio;LIBSOUNDIO_PATH"
    "libsndfile;LIBSNDFILE_PATH"
    "fftw;LIBFFTW3_PATH"
    "glew;GLEW_PATH"
    "glfw;GLFW_PATH"
    "libxml2;LIBXML2_PATH"
)

for lib in "${libs[@]}"; do
    IFS=";" read -r lib_name env_var <<< "$lib"
    package=$(package_name "$lib_name")

    lib_dir_name=${lib_name%%@*}

    path=$(check_library "$lib_dir_name")
    if [[ -n "$path" ]]; then
        echo "$lib_name Found at: $path"
    else
        echo "Installing $lib_name..."
        install_package "$package"
        path=$(get_lib_path "$package")
    fi
    echo "export $env_var=$path" >> .env
done

# Setup CLAP submodule
if [ -z "$(check_repo "$PWD/libs/clap")" ]; then
    git submodule update --init --recursive
fi
echo "export CLAP_PATH=$PWD/libs/clap" >> .env

echo "Setup completed successfully for $OS"
