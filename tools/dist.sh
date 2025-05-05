#!/bin/bash

DIST_DIR="dist"
EXECUTABLE="build/ylc"
CUSTOM_LIBS=("build/engine/libyalce_synth.so" "build/gui/libgui.so")

# Create distribution structure
mkdir -p "$DIST_DIR/bin"
mkdir -p "$DIST_DIR/lib"

# Copy executable
cp "$EXECUTABLE" "$DIST_DIR/bin/"

# Copy custom libraries
for lib in "${CUSTOM_LIBS[@]}"; do
  cp "$lib" "$DIST_DIR/lib/"
  basename=$(basename "$lib")
  install_name_tool -change "@rpath/$basename" "@executable_path/../lib/$basename" "$DIST_DIR/bin/$(basename "$EXECUTABLE")"
done

# Identify and copy external dependencies
DEPS=$(otool -L "$EXECUTABLE" | grep -v "@rpath" | grep "/opt/homebrew" | awk '{print $1}')
for dep in $DEPS; do
  basename=$(basename "$dep")
  cp "$dep" "$DIST_DIR/lib/"
  install_name_tool -change "$dep" "@executable_path/../lib/$basename" "$DIST_DIR/bin/$(basename "$EXECUTABLE")"
done

# Handle dependencies of custom libraries
for lib in "${CUSTOM_LIBS[@]}"; do
  basename=$(basename "$lib")
  LIB_DEPS=$(otool -L "$DIST_DIR/lib/$basename" | grep "/opt/homebrew" | awk '{print $1}')
  for dep in $LIB_DEPS; do
    dep_basename=$(basename "$dep")
    # Copy if not already copied
    if [ ! -f "$DIST_DIR/lib/$dep_basename" ]; then
      cp "$dep" "$DIST_DIR/lib/"
    fi
    install_name_tool -change "$dep" "@loader_path/$dep_basename" "$DIST_DIR/lib/$basename"
  done
done

# Create wrapper script
cat > "$DIST_DIR/ylc" << EOF
#!/bin/bash
DIR="\$( cd "\$( dirname "\${BASH_SOURCE[0]}" )" && pwd )"
"\$DIR/bin/$(basename "$EXECUTABLE")" "\$@"
EOF
chmod +x "$DIST_DIR/ylc"

# Create README
cat > "$DIST_DIR/README.md" << EOF
# YLC CLI Tool

## Installation
1. Extract this archive to a location of your choice
2. Run the \`ylc\` script in the root directory

## Requirements
- macOS 11.0 or later on Apple Silicon (M1 or newer)
EOF

echo "Distribution created at $DIST_DIR"
echo "To create a tarball: cd $DIST_DIR && tar -czf ../ylc-dist.tar.gz ."
