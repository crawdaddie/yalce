#!/bin/bash

set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

mkdir -p "$TMP_DIR/bin" "$TMP_DIR/libs/clap/.git"
cp "$ROOT/setup.sh" "$TMP_DIR/setup.sh"

cat > "$TMP_DIR/bin/brew" <<'EOF'
#!/bin/bash

if [[ "$1" == "--prefix" ]]; then
    if [[ -n "${2:-}" ]]; then
        echo "/fake/homebrew/opt/$2"
    else
        echo "/fake/homebrew"
    fi
    exit 0
fi

if [[ "$1" == "install" ]]; then
    echo "$2" >> "$BREW_LOG"
fi
EOF
chmod +x "$TMP_DIR/bin/brew"

BREW_LOG="$TMP_DIR/brew.log"
export BREW_LOG

PATH="$TMP_DIR/bin:$PATH" YLC_OS=macos \
    bash -c "cd '$TMP_DIR' && bash setup.sh" >/dev/null

grep -Fx 'export CPATH=/fake/homebrew/include' "$TMP_DIR/.env"
grep -Fx 'export LIBRARY_PATH=/fake/homebrew/lib' "$TMP_DIR/.env"
grep -Fx 'export SDL2_IMAGE_PATH=/fake/homebrew/opt/sdl2_image' "$TMP_DIR/.env"
grep -q 'echo "llvm@21"' "$TMP_DIR/setup.sh"
grep -Eq '^export LIBXML2_PATH=.+' "$TMP_DIR/.env"

for package in sdl2_image glew glfw; do
    grep -Fx "$package" "$BREW_LOG"
done

if grep -q 'declare -A' "$ROOT/setup.sh"; then
    echo 'setup.sh requires Bash 4 associative arrays' >&2
    exit 1
fi
