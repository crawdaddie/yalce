#include "common.h"
#include "ctx.h"
#include "raylib.h"
#include <stdlib.h>

void *create_window(void) {
  SetTraceLogLevel(LOG_ERROR);
  int screen_width = 400;
  int screen_height = 400;
  InitWindow(screen_width, screen_height,
             "oscilloscope [Audio Context DAC buffer]");
  Vector2 position[LAYOUT_CHANNELS] = {0, 0};
  // double prev_pos_y[LAYOUT_CHANNELS] = {0};

  Vector2 prev_pos[LAYOUT_CHANNELS] = {0, 0};

  SetTargetFPS(30); // Set our game to run at 30 frames-per-second
  //
  Ctx *ctx = get_audio_ctx();

  double *data = ctx->dac_buffer.buf;

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw the current buffer state proportionate to the screen
    prev_pos[0].x = 0.;
    prev_pos[1].x = 0.;

    for (int i = 0; i < screen_width; i++) {
      position[0].x = (float)i;
      position[1].x = (float)i;
      for (int ch = 0; ch < LAYOUT_CHANNELS; ch++) {
        position[ch].y = (2 * ch + 1) * screen_height / (LAYOUT_CHANNELS * 2) +
                         10 * data[ch + i * BUF_SIZE / screen_width];
        DrawLine(prev_pos[ch].x, prev_pos[ch].y, position[ch].x, position[ch].y,
                 RED);
        prev_pos[ch] = (Vector2){(float)(i), position[ch].y};
      }
    }
    EndDrawing();
  }
  CloseWindow();
  exit(0);
}
