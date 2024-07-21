# clang --target=wasm32-unknown-wasi \
#   --sysroot /tmp/wasi-libc \
#   -nostartfiles \
#   -Wl,--export-all \
#   -Wl,--no-entry \
#   -o yalce-fe/jit.wasm \
#   lang/backend_wasm/jit.c
#
clang --target=wasm32-unknown-wasi \
  --sysroot /tmp/wasi-libc \
  -nostartfiles \
  -Wl,--export-all \
  -Wl,--no-entry \
  -Ilang/ \
  -o yalce-fe/jit.wasm \
  lang/backend_wasm/jit.c \
  lang/parse.c \
  lang/y.tab.c \
  lang/lex.yy.c \
  lang/string_proc.c \
  lang/ht.c \
  lang/common.c



