#!/bin/bash

# Arch Linux installation script for YLC dependencies

echo "Installing YLC dependencies for Arch Linux..."

# Core build tools
sudo pacman -S --needed --noconfirm \
    base-devel \
    clang \
    llvm \
    llvm-libs \
    flex \
    bison \
    make \
    git

# Audio libraries  
sudo pacman -S --needed --noconfirm \
    libsoundio \
    libsndfile \
    fftw

# GUI libraries
sudo pacman -S --needed --noconfirm \
    sdl2 \
    sdl2_ttf \
    sdl2_gfx \
    sdl2_image \
    glew \
    glfw

# Math libraries
sudo pacman -S --needed --noconfirm \
    openblas \
    openmp

# System libraries
sudo pacman -S --needed --noconfirm \
    readline \
    libxml2

# Optional AUR packages (requires yay or another AUR helper)
if command -v yay &> /dev/null; then
    echo "Installing AUR packages with yay..."
    yay -S --needed --noconfirm \
        libsoundio
fi

# Copy Arch Linux environment file
cp .env.arch .env

# Verify LLVM installation
echo ""
echo "Verifying LLVM installation..."
if command -v llvm-config &> /dev/null; then
    echo "✓ llvm-config found at: $(which llvm-config)"
    echo "✓ LLVM version: $(llvm-config --version)"
elif command -v llvm-config-16 &> /dev/null; then
    echo "✓ llvm-config-16 found at: $(which llvm-config-16)"
    echo "✓ LLVM version: $(llvm-config-16 --version)"
elif command -v llvm-config-15 &> /dev/null; then
    echo "✓ llvm-config-15 found at: $(which llvm-config-15)"
    echo "✓ LLVM version: $(llvm-config-15 --version)"
else
    echo "⚠ Warning: No llvm-config found. You may need to install llvm-config manually."
fi

echo ""
echo "Dependencies installed successfully!"
echo "To build the project, run: make"
echo "For debug build, run: make debug"