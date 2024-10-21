#include "clap_host.h"
#include "clap/version.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>


// CLAP host callbacks
static const void *clap_host_get_extension(const struct clap_host *host,
                                           const char *extension_id) {
  // Implement as needed
  return NULL;
}

static void clap_host_request_restart(const struct clap_host *host) {
  // Implement as needed
}

static void clap_host_request_process(const struct clap_host *host) {
  // Implement as needed
}

static void clap_host_request_callback(const struct clap_host *host) {
  // Implement as needed
}

static const struct clap_host host = {
    // .clap_version = CLAP_VERSION,
    .host_data = NULL,
    .name = "Your Host Name",
    .vendor = "Your Vendor Name",
    .url = "Your URL",
    .version = "1.0.0",
    .get_extension = clap_host_get_extension,
    .request_restart = clap_host_request_restart,
    .request_process = clap_host_request_process,
    .request_callback = clap_host_request_callback};

Node *clap_node(const char *plugin_path, int num_inputs, Signal **inputs) {
    void *handle = dlopen(plugin_path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Failed to load plugin '%s': %s\n", plugin_path, dlerror());
        return NULL;
    }

    const struct clap_plugin_entry *entry = 
        (const struct clap_plugin_entry *)dlsym(handle, "clap_entry");
    if (!entry) {
        fprintf(stderr, "Unable to resolve entry point 'clap_entry' in '%s'\n", plugin_path);
        dlclose(handle);
        return NULL;
    }

    if (!entry->init(plugin_path)) {
        fprintf(stderr, "Failed to initialize CLAP entry point for '%s'\n", plugin_path);
        dlclose(handle);
        return NULL;
    }

    const struct clap_plugin_factory *factory = 
        (const struct clap_plugin_factory *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    if (!factory) {
        fprintf(stderr, "Failed to get CLAP plugin factory for '%s'\n", plugin_path);
        entry->deinit();
        dlclose(handle);
        return NULL;
    }

    uint32_t plugin_count = factory->get_plugin_count(factory);
    if (plugin_count == 0) {
        fprintf(stderr, "No CLAP plugins found in '%s'\n", plugin_path);
        entry->deinit();
        dlclose(handle);
        return NULL;
    }

    // For simplicity, we'll use the first plugin (index 0)
    const struct clap_plugin_descriptor *desc = 
        factory->get_plugin_descriptor(factory, 0);
    if (!desc) {
        fprintf(stderr, "Failed to get plugin descriptor for '%s'\n", plugin_path);
        entry->deinit();
        dlclose(handle);
        return NULL;
    }

    if (!clap_version_is_compatible(desc->clap_version)) {
        fprintf(stderr, "Incompatible CLAP version for '%s'. Plugin: %d.%d.%d, Host: %d.%d.%d\n",
                plugin_path, 
                desc->clap_version.major, desc->clap_version.minor, desc->clap_version.revision,
                CLAP_VERSION.major, CLAP_VERSION.minor, CLAP_VERSION.revision);
        entry->deinit();
        dlclose(handle);
        return NULL;
    }

    const struct clap_plugin *plugin = 
        factory->create_plugin(factory, &host, desc->id);
    if (!plugin) {
        fprintf(stderr, "Failed to create plugin instance for '%s' with id: %s\n", 
                plugin_path, desc->id);
        entry->deinit();
        dlclose(handle);
        return NULL;
    }

    if (!plugin->init(plugin)) {
        fprintf(stderr, "Failed to initialize plugin '%s' with id: %s\n", 
                plugin_path, desc->id);
        plugin->destroy(plugin);
        entry->deinit();
        dlclose(handle);
        return NULL;
    }
    // Activate the plugin
    if (!plugin->activate(plugin, 48000, 512, 1024)) {  // Example values, adjust as needed
        fprintf(stderr, "Failed to activate plugin '%s' with id: %s\n", 
                plugin_path, desc->id);
        plugin->destroy(plugin);
        entry->deinit();
        dlclose(handle);
        return NULL;
    }

    __clap_plugin_state *state = malloc(sizeof(__clap_plugin_state));
    state->plugin = plugin;
    state->desc = desc;
    state->instance = handle;
    state->entry = entry;

    Node *node = node_new(state, (node_perform)clap_perform, num_inputs, inputs);
    return node;
}

void clap_perform(Node *node, int nframes, double samplerate) {
  __clap_plugin_state *state = (__clap_plugin_state *)node->state;
  const struct clap_plugin *plugin = state->plugin;

  // Prepare audio buffers
  struct clap_audio_buffer audio_inputs = {
      .channel_count = node->num_ins,
      .latency = 0,
      .constant_mask = 0,
  };
  struct clap_audio_buffer audio_outputs = {
      .channel_count = 1, // Assuming mono output
      .latency = 0,
      .constant_mask = 0,
  };

  float *input_buffers[node->num_ins];
  float output_buffer[nframes];

  for (int i = 0; i < node->num_ins; i++) {
    input_buffers[i] = malloc(nframes * sizeof(float));
    for (int j = 0; j < nframes; j++) {
      input_buffers[i][j] = (float)node->ins[i].buf[j];
    }
  }

  audio_inputs.data32 = input_buffers;
  audio_outputs.data32 = &output_buffer;

  // Process audio
  struct clap_process process = {
      .steady_time = -1,
      .frames_count = nframes,
      .transport = NULL, // Implement if needed
      .audio_inputs = &audio_inputs,
      .audio_outputs = &audio_outputs,
      .audio_inputs_count = 1,
      .audio_outputs_count = 1,
  };

  plugin->process(plugin, &process);

  // Copy output back to node
  for (int i = 0; i < nframes; i++) {
    node->out.buf[i] = (double)output_buffer[i];
  }

  // Clean up
  for (int i = 0; i < node->num_ins; i++) {
    free(input_buffers[i]);
  }
}

void clap_node_destroy(Node *node) {
  if (node && node->state) {
    __clap_plugin_state *state = (__clap_plugin_state *)node->state;
    state->plugin->deactivate(state->plugin);
    state->plugin->destroy(state->plugin);
    dlclose(state->instance);
    free(state);
  }
  // Assuming there's a function to free the Node structure
  // node_free(node);
}
