#ifndef YLC_QWEN_TOKENIZER_H
#define YLC_QWEN_TOKENIZER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HfTokenizer HfTokenizer;

HfTokenizer *hf_tokenizer_load(const char *filename);

/* Returns token count; returns -required_count when output is too small. */
int64_t hf_tokenizer_encode(HfTokenizer *tokenizer, const char *input,
                            int32_t *output, size_t capacity);

void hf_tokenizer_free(HfTokenizer *tokenizer);

#ifdef __cplusplus
}
#endif

#endif
