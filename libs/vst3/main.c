#include "./vst3.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 512
#define NUM_CHANNELS 2

// Simple audio processing test with a sine wave input
void process_audio_test(VST3_Plugin *plugin) {
  float *input_buffer[NUM_CHANNELS];
  float *output_buffer[NUM_CHANNELS];

  // Allocate buffers
  for (int i = 0; i < NUM_CHANNELS; i++) {
    input_buffer[i] = (float *)malloc(BUFFER_SIZE * sizeof(float));
    output_buffer[i] = (float *)malloc(BUFFER_SIZE * sizeof(float));

    if (!input_buffer[i] || !output_buffer[i]) {
      printf("Error: Failed to allocate audio buffers\n");
      return;
    }
  }

  // Create a simple sine wave input
  for (int i = 0; i < BUFFER_SIZE; i++) {
    float t = (float)i / SAMPLE_RATE;
    float sine_value =
        0.5f * sinf(2.0f * 3.14159f * 440.0f * t); // 440 Hz sine wave

    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      input_buffer[ch][i] = sine_value;
      output_buffer[ch][i] = 0.0f; // Clear output buffer
    }
  }

  // Set up audio buffer structure
  VST3_AudioBuffer buffer;
  buffer.inputs = input_buffer;
  buffer.outputs = output_buffer;
  buffer.num_input_channels = NUM_CHANNELS;
  buffer.num_output_channels = NUM_CHANNELS;
  buffer.num_samples = BUFFER_SIZE;

  // Start processing
  VST3_Result result = vst3_plugin_start_processing(plugin);
  if (result != VST3_OK) {
    printf("Error: Failed to start processing\n");
    goto cleanup;
  }

  // Process audio
  result = vst3_plugin_process(plugin, &buffer);
  if (result != VST3_OK) {
    printf("Error: Failed to process audio\n");
    goto cleanup;
  }

  // Print first few samples for demonstration
  printf("First 10 samples of processed audio:\n");
  for (int i = 0; i < 10; i++) {
    printf("Sample %d: ", i);
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      printf("Ch%d = %.6f ", ch, output_buffer[ch][i]);
    }
    printf("\n");
  }

  // Stop processing
  vst3_plugin_stop_processing(plugin);

cleanup:
  // Free buffers
  for (int i = 0; i < NUM_CHANNELS; i++) {
    free(input_buffer[i]);
    free(output_buffer[i]);
  }
}

// Print parameter information
void print_parameters(VST3_Plugin *plugin) {
  int num_params = vst3_plugin_get_num_params(plugin);
  printf("Plugin has %d parameters:\n", num_params);

  for (int i = 0; i < num_params; i++) {
    VST3_ParamInfo param_info;
    VST3_Result result = vst3_plugin_get_param_info(plugin, i, &param_info);

    if (result == VST3_OK) {
      VST3_ParamValue param_value;
      vst3_plugin_get_param_value(plugin, param_info.id, &param_value);

      printf("Parameter %d: ID=%d, Value=%.2f %s\n", i, param_info.id,
             param_value.value, param_value.units);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <path_to_vst3_plugin>\n", argv[0]);
    return 1;
  }

  const char *plugin_path = argv[1];
  printf("Loading VST3 plugin: %s\n", plugin_path);

  // Create host
  VST3_Host *host = vst3_host_create(SAMPLE_RATE, BUFFER_SIZE);
  if (!host) {
    printf("Error: Failed to create VST3 host\n");
    return 1;
  }

  // Load plugin
  VST3_Plugin *plugin = vst3_plugin_load(host, plugin_path);
  if (!plugin) {
    printf("Error: Failed to load VST3 plugin\n");
    vst3_host_free(host);
    return 1;
  }

  // Get plugin info
  VST3_PluginInfo info;
  VST3_Result result = vst3_plugin_get_info(plugin, &info);
  if (result != VST3_OK) {
    printf("Error: Failed to get plugin info\n");
    vst3_plugin_unload(plugin);
    vst3_host_free(host);
    return 1;
  }

  // Print plugin info
  printf("Plugin Info:\n");
  printf("  Name: %s\n", info.name);
  printf("  Vendor: %s\n", info.vendor);
  printf("  Inputs: %d\n", info.num_inputs);
  printf("  Outputs: %d\n", info.num_outputs);
  printf("  Parameters: %d\n", info.num_params);
  printf("  ID: %d\n", info.unique_id);

  // Initialize plugin
  result = vst3_plugin_init(plugin, SAMPLE_RATE, BUFFER_SIZE);
  if (result != VST3_OK) {
    printf("Error: Failed to initialize plugin\n");
    vst3_plugin_unload(plugin);
    vst3_host_free(host);
    return 1;
  }

  // Print parameters
  print_parameters(plugin);

  // Process some audio
  process_audio_test(plugin);

  // Clean up
  vst3_plugin_unload(plugin);
  vst3_host_free(host);

  printf("VST3 plugin test completed successfully\n");
  return 0;
}
