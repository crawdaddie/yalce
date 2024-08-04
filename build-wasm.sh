WASI_SDK_PATH=~/Desktop/wasi-sdk-23.0-arm64-macos
$WASI_SDK_PATH/bin/clang \
  --target=wasm32-wasi \
  -O3 \
  -nostartfiles \
  -Wl,--no-entry \
  -Wl,--export-all \
  -I$WASI_SDK_PATH/share/wasi-sysroot/include \
  -I./lang/ \
  -o docs/jit.wasm \
  $(find lang -name '*.c' ! -name 'input.c' ! -name 'synths.c' ! -name 'main.c' ! -path '*/backend_llvm/*') \
  -Wl,--allow-undefined
