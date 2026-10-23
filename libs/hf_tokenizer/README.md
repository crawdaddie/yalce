# Hugging Face tokenizer bridge

Build and test:

```sh
cargo build --release
cargo test
```

The library is `target/release/libhf_tokenizer.so`.

- `hf_tokenizer_load(path)` loads a `tokenizer.json`.
- `hf_tokenizer_encode` writes IDs into a caller-owned `int32_t` buffer.
- `hf_tokenizer_decode(handle, token, output, capacity)` decodes one ID into
  UTF-8 bytes, preserving spaces and special tokens. Pass null output to query
  the byte count, then allocate that many bytes. No trailing NUL is written.
  Invalid inputs or insufficient capacity return `-1`.
- `hf_tokenizer_decode_ids` decodes an ID array together, preserving UTF-8
  characters spanning multiple tokens. It uses the same byte-count convention.
- `hf_tokenizer_free(handle)` releases the tokenizer.
