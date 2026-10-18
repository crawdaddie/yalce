# Qwen tokenizer bridge

Build:

```sh
cargo build --release
```

The library is `target/release/libqwen_tokenizer.so`.

It loads a Hugging Face `tokenizer.json` and writes token IDs into a caller-
owned `int64_t` buffer. A negative return means the buffer was too small and
its absolute value is the required capacity.
