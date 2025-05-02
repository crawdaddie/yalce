#ifndef VST3_C_WRAPPER_H
#define VST3_C_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* Opaque handle types */
typedef struct VST3_Host_Impl VST3_Host;
typedef struct VST3_Plugin_Impl VST3_Plugin;
// typedef struct VST3_ParamInfo_Impl VST3_ParamInfo;

/* Error codes */
typedef enum {
    VST3_OK = 0,
    VST3_ERROR_FAILED = -1,
    VST3_ERROR_INVALID_ARG = -2,
    VST3_ERROR_NOT_IMPLEMENTED = -3,
    VST3_ERROR_PLUGIN_NOT_FOUND = -4,
    VST3_ERROR_PLUGIN_LOAD_FAILED = -5,
    VST3_ERROR_FACTORY_NOT_FOUND = -6,
    VST3_ERROR_CLASS_NOT_FOUND = -7,
    VST3_ERROR_COMPONENT_NOT_FOUND = -8,
    VST3_ERROR_PROCESSOR_NOT_FOUND = -9,
    VST3_ERROR_CONTROLLER_NOT_FOUND = -10
} VST3_Result;

/* Audio buffer structure */
typedef struct {
    float** inputs;           /* Array of input channel buffers */
    float** outputs;          /* Array of output channel buffers */
    int num_input_channels;   /* Number of input channels */
    int num_output_channels;  /* Number of output channels */
    int num_samples;          /* Number of samples per buffer */
} VST3_AudioBuffer;

/* Plugin information */
typedef struct {
    char name[128];
    char vendor[128];
    int num_inputs;
    int num_outputs;
    int num_params;
    int unique_id;
    int version;
} VST3_PluginInfo;

/* Parameter information */
typedef struct {
    int id;
    float defaultValue;
    int stepCount;
    int flags;
    char title[128];
    char units[32];
} VST3_ParamInfo;

/* Parameter value */
typedef struct {
    int id;
    float value;
    char title[128];
    char units[32];
} VST3_ParamValue;

/* Host functions */

/**
 * Create a VST3 host environment
 * @param sample_rate The sample rate to initialize with
 * @param block_size The audio block size
 * @return A handle to the host environment, or NULL on failure
 */
VST3_Host* vst3_host_create(double sample_rate, int block_size);

/**
 * Free a VST3 host environment
 * @param host The host handle to free
 */
void vst3_host_free(VST3_Host* host);

/**
 * Load a VST3 plugin
 * @param host The host environment
 * @param path Path to the VST3 module file
 * @return A handle to the loaded plugin, or NULL on failure
 */
VST3_Plugin* vst3_plugin_load(VST3_Host* host, const char* path);

/**
 * Unload a VST3 plugin and free resources
 * @param plugin The plugin handle to unload
 */
void vst3_plugin_unload(VST3_Plugin* plugin);

/**
 * Get information about a loaded plugin
 * @param plugin The plugin handle
 * @param info Pointer to a structure to receive the information
 * @return VST3_OK on success, error code otherwise
 */
VST3_Result vst3_plugin_get_info(VST3_Plugin* plugin, VST3_PluginInfo* info);

/**
 * Initialize a plugin for processing
 * @param plugin The plugin handle
 * @param sample_rate The sample rate to use
 * @param block_size The maximum block size
 * @return VST3_OK on success, error code otherwise
 */
VST3_Result vst3_plugin_init(VST3_Plugin* plugin, double sample_rate, int block_size);

/**
 * Process audio through the plugin
 * @param plugin The plugin handle
 * @param buffer Audio buffer containing input and output pointers
 * @return VST3_OK on success, error code otherwise
 */
VST3_Result vst3_plugin_process(VST3_Plugin* plugin, VST3_AudioBuffer* buffer);

/**
 * Get the number of parameters in the plugin
 * @param plugin The plugin handle
 * @return Number of parameters, or -1 on error
 */
int vst3_plugin_get_num_params(VST3_Plugin* plugin);

/**
 * Get information about a parameter
 * @param plugin The plugin handle
 * @param param_index The parameter index
 * @param info Pointer to a structure to receive the information
 * @return VST3_OK on success, error code otherwise
 */
VST3_Result vst3_plugin_get_param_info(VST3_Plugin* plugin, int param_index, VST3_ParamInfo* info);

/**
 * Get a parameter's current value
 * @param plugin The plugin handle
 * @param param_id The parameter ID
 * @param value Pointer to receive the parameter value
 * @return VST3_OK on success, error code otherwise
 */
VST3_Result vst3_plugin_get_param_value(VST3_Plugin* plugin, int param_id, VST3_ParamValue* value);

/**
 * Set a parameter's value
 * @param plugin The plugin handle
 * @param param_id The parameter ID
 * @param value The new parameter value
 * @return VST3_OK on success, error code otherwise
 */
VST3_Result vst3_plugin_set_param_value(VST3_Plugin* plugin, int param_id, float value);

/**
 * Start audio processing
 * @param plugin The plugin handle
 * @return VST3_OK on success, error code otherwise
 */
VST3_Result vst3_plugin_start_processing(VST3_Plugin* plugin);

/**
 * Stop audio processing
 * @param plugin The plugin handle
 * @return VST3_OK on success, error code otherwise
 */
VST3_Result vst3_plugin_stop_processing(VST3_Plugin* plugin);

#ifdef __cplusplus
}
#endif

#endif /* VST3_C_WRAPPER_H */
