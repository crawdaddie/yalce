#include "oscilloscope.h"

#include <unistd.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define MAX_AMPLITUDE 1.0

static double osc_scope_buf[BUF_SIZE];
static pthread_mutex_t buffer_mutex;
void set_osc_scope_buf(int frame, double value) {
  osc_scope_buf[frame % BUF_SIZE] = value;
}

void add_osc_scope_buf(int frame, double value) {
  osc_scope_buf[frame % BUF_SIZE] += value;
}

// Define the function for updating the oscilloscope buffer
static void *update_osc_scope_buffer(void *arg) {
  while (1) {
    // Generate a new sample and add it to the buffer
    double sample = sin((double)rand() / RAND_MAX * M_PI * 2) * MAX_AMPLITUDE;
    pthread_mutex_lock(&buffer_mutex);
    for (int i = 0; i < BUF_SIZE - 1; i++) {
      osc_scope_buf[i] = osc_scope_buf[i + 1];
    }
    osc_scope_buf[BUF_SIZE - 1] = sample;
    pthread_mutex_unlock(&buffer_mutex);
    // Sleep for a short time to simulate real-time sampling
    usleep(10000);
  }
  return NULL;
}

// Define the function for drawing the waveform on the tigr window
static void draw_waveform_on_window(Tigr *screen) {
  // Lock the buffer to prevent concurrent access
  /* pthread_mutex_lock(&buffer_mutex); */
  // Clear the screen
  tigrClear(screen, tigrRGB(0, 0, 0));
  // Draw the waveform
  for (int i = 0; i < BUF_SIZE - 1; i++) {
    int x1 = i * WINDOW_WIDTH / BUF_SIZE;
    int x2 = (i + 1) * WINDOW_WIDTH / BUF_SIZE;
    int y1 = WINDOW_HEIGHT / 2 -
             osc_scope_buf[i] * WINDOW_HEIGHT / (2 * MAX_AMPLITUDE);
    int y2 = WINDOW_HEIGHT / 2 -
             osc_scope_buf[i + 1] * WINDOW_HEIGHT / (2 * MAX_AMPLITUDE);
    tigrLine(screen, x1, y1, x2, y2, tigrRGB(255, 255, 255));

    /* usleep(10000); */
  }
  usleep(10666);
  // Unlock the buffer
  /* pthread_mutex_unlock(&buffer_mutex); */
}

// Define the main function
int oscilloscope() {
  // Initialize the mutex
  /* pthread_mutex_init(&buffer_mutex, NULL); */

  // Create the update buffer thread
  /* pthread_t update_buffer_thread; */
  /* pthread_create(&update_buffer_thread, NULL, update_osc_scope_buffer, NULL);
   */

  // Create the tigr window
  Tigr *screen =
      tigrWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Oscilloscope", TIGR_FIXED);

  // Draw the waveform on the tigr window
  while (!tigrClosed(screen)) {
    draw_waveform_on_window(screen);
    tigrUpdate(screen);
  }

  // Clean up
  /* pthread_mutex_destroy(&buffer_mutex); */
  tigrFree(screen);
  return 0;
}
