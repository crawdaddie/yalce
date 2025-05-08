// clang -Wall -g main.c src/CocoaBridge.m -o vst_tester -framework Cocoa
// -framework Foundation -I/path/to/vst/headers -I./src -ObjC -framework
// AudioUnit -framework CoreAudio -std=c11
#include "src/VSTPlugin.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Function to handle user input while running
void printCommands() {
  printf("\nAvailable commands:\n");
  printf("  q - Quit\n");
  printf("  p - Process some audio\n");
  printf("  g - Toggle GUI (open/close)\n");
  printf("  h - Show this help\n");
  printf("Command: ");
  fflush(stdout);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <path_to_vst_plugin>\n", argv[0]);
    return 1;
  }

  const char *plugin_path = argv[1];
  printf("Initializing VST library...\n");

  // Initialize the VST library
  VSTError err = vst_initialize();
  if (err != VST_ERR_OK) {
    printf("Failed to initialize VST library: %d\n", err);
    return 1;
  }

  printf("Loading plugin: %s\n", plugin_path);

  // Load the plugin
  VSTPluginHandle plugin = NULL;
  err = vst_load_plugin(plugin_path, &plugin);
  if (err != VST_ERR_OK) {
    printf("Failed to load plugin: %d\n", err);
    vst_terminate();
    return 1;
  }

  // Get plugin type
  VSTPluginType type = vst_get_plugin_type(plugin);
  printf("Plugin type: %s\n",
         type == VST_PLUGIN_TYPE_VST2
             ? "VST2"
             : (type == VST_PLUGIN_TYPE_VST3 ? "VST3" : "Unknown"));

  // Get plugin name
  char name[128] = {0};
  err = vst_get_plugin_name(plugin, name, sizeof(name));
  if (err == VST_ERR_OK) {
    printf("Plugin name: %s\n", name);
  } else {
    printf("Failed to get plugin name: %d\n", err);
  }

  // Get parameter count
  uint32_t param_count = 0;
  err = vst_get_parameter_count(plugin, &param_count);
  if (err == VST_ERR_OK) {
    printf("Plugin has %u parameters\n", param_count);

    // List first 5 parameters
    printf("First 5 parameters (or fewer if not available):\n");
    for (uint32_t i = 0; i < param_count && i < 5; i++) {
      VSTParameterInfo param_info;
      err = vst_get_parameter_info(plugin, i, &param_info);
      if (err == VST_ERR_OK) {
        printf("  Parameter %u: %s (%s) [%.2f-%.2f] default: %.2f\n", i,
               param_info.name, param_info.label, param_info.minValue,
               param_info.maxValue, param_info.defaultValue);
      } else {
        printf("  Failed to get info for parameter %u: %d\n", i, err);
      }
    }
  } else {
    printf("Failed to get parameter count: %d\n", err);
  }

  // Setup processing
  printf("Setting up audio processing...\n");
  VSTProcessSetup process_setup = {.sampleRate = 44100.0f,
                                   .maxBlockSize = 512,
                                   .numInputChannels = 2,
                                   .numOutputChannels = 2};

  err = vst_setup_processing(plugin, &process_setup);
  if (err != VST_ERR_OK) {
    printf("Failed to setup processing: %d\n", err);
  } else {
    printf("Audio processing setup successfully\n");
  }

  // Check if plugin supports bypass
  printf("Plugin supports bypass: %s\n",
         vst_supports_feature(plugin, "bypass") ? "Yes" : "No");

  // Check if plugin supports MIDI
  printf("Plugin supports MIDI: %s\n",
         vst_supports_feature(plugin, "midiEvents") ? "Yes" : "No");

  // Check if plugin has an editor
  int has_editor;
  vst_has_editor(plugin, &has_editor);
  printf("Plugin has GUI: %s\n", has_editor ? "Yes" : "No");

  // Create and allocate buffers for audio processing
  float **inputs = (float **)malloc(2 * sizeof(float *));
  float **outputs = (float **)malloc(2 * sizeof(float *));

  for (int i = 0; i < 2; i++) {
    inputs[i] = (float *)calloc(512, sizeof(float));
    outputs[i] = (float *)calloc(512, sizeof(float));

    // Generate a simple sine wave for testing
    for (int j = 0; j < 512; j++) {
      inputs[i][j] = 0.5f * sinf(2.0f * 3.14159f * 440.0f * j / 44100.0f);
    }
  }

  // Main interaction loop
  bool editor_open = false;
  bool running = true;

  // Open editor if available
  if (has_editor) {
    printf("Opening GUI...\n");
    err = vst_open_editor(plugin);
    if (err == VST_ERR_OK) {
      printf("GUI opened successfully\n");
      editor_open = true;
    } else {
      printf("Failed to open GUI: %d\n", err);
    }
  }

  // Command loop
  printCommands();

  char cmd[64];
  while (running) {
    // Process editor events if it's open
    // if (editor_open) {
    //   vst_process_editor_events(plugin);
    //
    //   // Check if editor was closed by user
    //   if (!vst_is_editor_open(plugin)) {
    //     printf("GUI was closed\n");
    //     editor_open = false;
    //     printCommands();
    //   }
    // }

    // Check for user input (non-blocking)
    if (fgets(cmd, sizeof(cmd), stdin)) {
      if (cmd[0] == 'q') {
        running = false;
        printf("Exiting...\n");
      } else if (cmd[0] == 'p') {
        printf("Processing audio...\n");
        err = vst_process_audio(plugin, (const float **)inputs, outputs, 512);
        if (err == VST_ERR_OK) {
          printf("Audio processed successfully\n");

          // Print first few samples of output for verification
          printf("First 5 samples of output channel 0:\n");
          for (int i = 0; i < 5; i++) {
            printf("  Sample %d: %.6f\n", i, outputs[0][i]);
          }
        } else {
          printf("Failed to process audio: %d\n", err);
        }
        printCommands();
      } else if (cmd[0] == 'g') {
        if (has_editor) {
          if (editor_open) {
            printf("Closing GUI...\n");
            err = vst_close_editor(plugin);
            if (err == VST_ERR_OK) {
              printf("GUI closed\n");
              editor_open = false;
            } else {
              printf("Failed to close GUI: %d\n", err);
            }
          } else {
            printf("Opening GUI...\n");
            err = vst_open_editor(plugin);
            if (err == VST_ERR_OK) {
              printf("GUI opened\n");
              editor_open = true;
            } else {
              printf("Failed to open GUI: %d\n", err);
            }
          }
        } else {
          printf("This plugin does not have a GUI\n");
        }
        printCommands();
      } else if (cmd[0] == 'h') {
        printCommands();
      }
    }

// Small sleep to prevent CPU hogging
#ifdef _WIN32
    Sleep(10); // Windows
#else
    usleep(10000); // Unix/Linux (10ms)
#endif
  }

  // Close editor if open
  if (editor_open) {
    printf("Closing GUI...\n");
    err = vst_close_editor(plugin);
    if (err != VST_ERR_OK) {
      printf("Failed to close GUI: %d\n", err);
    }
  }

  // Free buffers
  for (int i = 0; i < 2; i++) {
    free(inputs[i]);
    free(outputs[i]);
  }
  free(inputs);
  free(outputs);

  // Unload plugin
  printf("Unloading plugin...\n");
  err = vst_unload_plugin(plugin);
  if (err != VST_ERR_OK) {
    printf("Failed to unload plugin: %d\n", err);
  }

  // Terminate library
  printf("Terminating VST library...\n");
  vst_terminate();

  printf("Test completed\n");

  return 0;
}
