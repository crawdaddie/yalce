#include "msg_queue.h"
#include "ctx.h"
#include "scheduling.h"
#include <stdio.h>
#include <stdlib.h>

void push_msg(msg_queue *queue, scheduler_msg msg) {
  if (queue->num_msgs == MSG_QUEUE_MAX_SIZE) {
    printf("Error: Command FIFO full\n");
    return;
  }

  *(queue->buffer + queue->write_ptr) = msg;
  queue->write_ptr = (queue->write_ptr + 1) % MSG_QUEUE_MAX_SIZE;
  queue->num_msgs++;
}

scheduler_msg pop_msg(msg_queue *queue) {
  scheduler_msg msg = *(queue->buffer + queue->read_ptr);
  queue->read_ptr = (queue->read_ptr + 1) % MSG_QUEUE_MAX_SIZE;
  queue->num_msgs--;
  return msg;
}

scheduler_msg *create_bundle(int length) {
  return malloc(sizeof(scheduler_msg) * length);
}

int get_write_ptr() {
  Ctx *ctx = get_audio_ctx();
  return ctx->msg_queue.write_ptr;
}
void update_bundle(int write_ptr) {
  Ctx *ctx = get_audio_ctx();
  msg_queue queue = ctx->msg_queue;

  int offset = (int)(get_block_diff() * ctx->sample_rate);
  // printf("update bundle %d %d\n", write_ptr, queue.write_ptr);
  while (write_ptr != queue.write_ptr) {
    queue.buffer[write_ptr].frame_offset = offset;
    // printf("updating %d\n", write_ptr);
    write_ptr = (write_ptr + 1) % MSG_QUEUE_MAX_SIZE;
  }
}
