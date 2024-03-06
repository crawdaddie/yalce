#include "common.h"
#include "ctx.h"
#include "raylib.h"
#include <fftw3.h>
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
#define FFT_SIZE 512

void *create_spectrogram_window(void) {
  SetTraceLogLevel(LOG_ERROR);
  int screen_width = 400;
  int screen_height = 400;
  InitWindow(screen_width, screen_height,
             "oscilloscope [Audio Context DAC buffer]");

  SetTargetFPS(30); // Set our game to run at 30 frames-per-second

  Ctx *ctx = get_audio_ctx();
  double *data = ctx->dac_buffer.buf;
  double summed_audio[FFT_SIZE]; // summed stereo channels

  fftw_complex *in, *out;
  fftw_plan p;

  in = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
  out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);

  // Create FFTW plan
  p = fftw_plan_dft_1d(FFT_SIZE, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);


    for (int j = 0; j < FFT_SIZE; j++) {
      summed_audio[j] = 0.5 * (data[j] + data[j + 1]);
    }

    // Prepare input for FFT
    for (int i = 0; i < FFT_SIZE; i++) {
      in[i][0] = summed_audio[i]; // Real part
      in[i][1] =
          0; // Imaginary part (we're dealing with real data, so set this to 0)
    }

    // Perform FFT
    fftw_execute(p);

    // Compute magnitude spectrum
    double magnitude[FFT_SIZE];
    for (int i = 0; i < FFT_SIZE; i++) {
      magnitude[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
    }

    // Plot on a log scale
    int log_index;
    for (int i = 1; i < screen_width; i++) {

      log_index = (int)(exp((i * log(FFT_SIZE / 2)) / screen_width));
      DrawLine(i - 1, screen_height, i,
               screen_height - 10 * magnitude[log_index],
               (Color){230, 41, 55, 128});
    }

    EndDrawing();
  }
  CloseWindow();
  // Free memory and destroy plan
  fftw_destroy_plan(p);
  fftw_free(in);
  fftw_free(out);
  exit(0);
}
