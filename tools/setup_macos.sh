#!/bin/bash

# Update Homebrew
brew update

# Install main dependencies
brew install sdl2 sdl2_ttf sdl2_gfx readline llvm@16 libsoundio libsndfile fftw

# Determine Homebrew prefix
BREW_PREFIX=$(brew --prefix)

# Create environment file with correct paths
cat > .env << EOF
export CPATH=$BREW_PREFIX/include
export LIBRARY_PATH=$BREW_PREFIX/lib
export LLVM_PATH=$BREW_PREFIX/opt/llvm@16
export SDL2_PATH=$BREW_PREFIX/opt/sdl2
export SDL2_TTF_PATH=$BREW_PREFIX/opt/sdl2_ttf
export SDL2_GFX_PATH=$BREW_PREFIX/opt/sdl2_gfx
export READLINE_PREFIX=$BREW_PREFIX/opt/readline
export LIBSOUNDIO_PATH=$BREW_PREFIX/opt/libsoundio
export LIBSNDFILE_PATH=$BREW_PREFIX/opt/libsndfile
export LIBFFTW3_PATH=$BREW_PREFIX/opt/fftw
EOF
