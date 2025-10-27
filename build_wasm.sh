WASI_SDK_PATH=$PWD/wasi-sdk-27.0-arm64-macos
BINARYEN_PATH=$HOME/projects/sound/binaryen

$WASI_SDK_PATH/bin/clang \
  --target=wasm32-wasi \
  -O3 \
  -nostartfiles \
  -Wl,--no-entry \
  -Wl,--export-all \
  -I$WASI_SDK_PATH/share/wasi-sysroot/include \
  -I./lang/ \
  -o docs/web/jit.wasm \
  $(find lang -name '*.c' ! -name 'input.c' ! -name 'synths.c' ! -name 'main.c' ! -name 'ylc_stdlib.c' ! -path '*/backend_llvm/*') \
  -Wl,--allow-undefined
