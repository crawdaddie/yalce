#! /bin/bash
check_library() {
    local lib_name=$1
    for path in "/usr/include/$lib_name" "/usr/local/include/$lib_name" "/opt/homebrew/opt/$lib_name"; do
        [ -d "$path" ] && echo "$path" && return
    done
    echo ""
}

check_repo() {
    [ -f "$1/.git" ] && echo "$1" || echo ""
}

# Initialize .env with default paths
cat > .env << EOF
export CPATH=/opt/homebrew/include
export LIBRARY_PATH=/opt/homebrew/lib
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
)

# Install and configure libraries
for lib in "${libs[@]}"; do
    IFS=";" read -r lib_name env_var <<< "$lib"
    
    if path=$(check_library "$lib_name"); then
        echo "$lib_name Found at: $path"
    else
        brew install "$lib_name"
        path=$(brew --prefix "$lib_name")
    fi
    echo "export $env_var=$path" >> .env
done

# Setup CLAP submodule
if [ -z "$(check_repo "$PWD/libs/clap")" ]; then
    git submodule update --init --recursive
fi
echo "export CLAP_PATH=$PWD/libs/clap" >> .env
