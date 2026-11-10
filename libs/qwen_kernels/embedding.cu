#include <stdint.h>

extern "C" __global__ void gather_embedding(const uint16_t *embedding,
                                            const int32_t *token_ids,
                                            uint16_t *output, int32_t hidden) {
  int index = blockIdx.x * blockDim.x + threadIdx.x;
  int token = blockIdx.y;

  if (index >= hidden) {
    return;
  }

  output[token * hidden + index] =
      embedding[(int64_t)token_ids[token] * hidden + index];
}
