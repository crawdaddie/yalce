#include "raylib.h"

#include <math.h>   // Required for: sinf()
#include <stdlib.h> // Required for: malloc(), free()

#define MAX_SAMPLES 512
#define MAX_SAMPLES_PER_UPDATE 4096

int main(void) {
  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight,
             "raylib [audio] example - raw audio streaming");

  SetAudioStreamBufferSizeDefault(MAX_SAMPLES_PER_UPDATE);

  short *data = (short *)malloc(sizeof(short) * MAX_SAMPLES);

  // Frame buffer, describing the waveform when repeated over the course of a
  // frame
  short *writeBuf = (short *)malloc(sizeof(short) * MAX_SAMPLES_PER_UPDATE);

  int waveLength = 1;

  Vector2 position = {0, 0};

  SetTargetFPS(30); // Set our game to run at 30 frames-per-second
  //--------------------------------------------------------------------------------------

  // Main game loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawText(TextFormat("stuff "), GetScreenWidth() - 220, 10, 20, RED);

    // Draw the current buffer state proportionate to the screen
    for (int i = 0; i < screenWidth; i++) {
      position.x = (float)i;
      position.y = 250 + 50 * data[i * MAX_SAMPLES / screenWidth] / 32000.0f;

      DrawPixelV(position, RED);
    }

    EndDrawing();
  }

  free(data);     // Unload sine wave data
  free(writeBuf); // Unload write buffer

  CloseWindow(); // Close window and OpenGL context
  return 0;
}
