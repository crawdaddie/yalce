# Building YLC on Arch Linux

This guide provides instructions for building YLC (yalce_synth) on Arch Linux.

## Quick Start

1. Install dependencies:
   ```bash
   ./install-arch.sh
   ```

2. Build the project:
   ```bash
   make
   ```

## Manual Installation

If you prefer to install dependencies manually:

### Core Build Tools
```bash
sudo pacman -S base-devel clang llvm flex bison make git
```

### Audio Libraries
```bash
sudo pacman -S libsoundio libsndfile fftw
```

### GUI Libraries  
```bash
sudo pacman -S sdl2 sdl2_ttf sdl2_gfx sdl2_image glew glfw
```

### Math Libraries
```bash
sudo pacman -S openblas openmp
```

### System Libraries
```bash
sudo pacman -S readline libxml2
```

### Environment Setup

Copy the Arch Linux environment configuration:
```bash
cp .env.arch .env
```

## Building

### Standard Build
```bash
make
```

### Debug Build
```bash
make debug
```

### Build Components Separately
```bash
# Build just the audio engine
make engine

# Build just the GUI
make gui

# Clean build artifacts
make clean
```

## Testing

```bash
# Run all tests
make test

# Test specific components
make test_parse
make test_typecheck
make test_scripts
make audio_test
```

## Usage

```bash
# Interactive REPL
./build/ylc -i

# Run a YLC file
./build/ylc filename.ylc

# Run with GUI
./build/ylc --gui
```

## Known Issues

- The project was originally developed for macOS, so some audio features may require additional Linux-specific libraries
- VST plugin support may need additional configuration for Linux VST hosts
- Some GUI features may behave differently on Linux compared to macOS

## Dependencies

The build system automatically detects Linux vs macOS and adjusts:
- Uses standard Linux library paths (`/usr/lib`, `/usr/include`)
- Replaces macOS frameworks with Linux equivalents (e.g., `-framework OpenGL` becomes `-lGL`)
- Uses `ldd` instead of `otool` for dependency checking
- Uses Linux-style RPATH (`$ORIGIN`) instead of macOS style (`@executable_path`)