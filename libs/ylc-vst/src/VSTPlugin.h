#ifndef YLC_VST_PLUGIN_H
#define YLC_VST_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Opaque handle to a VST plugin instance
 */
typedef struct VSTPluginInstance_t *VSTPluginHandle;

/**
 * @brief Audio processing format parameters
 */
typedef struct {
  float sampleRate;
  uint32_t maxBlockSize;
  uint32_t numInputChannels;
  uint32_t numOutputChannels;
} VSTProcessSetup;

/**
 * @brief Parameter information
 */
typedef struct {
  uint32_t id;
  char name[128];
  char label[128];
  float minValue;
  float maxValue;
  float defaultValue;
} VSTParameterInfo;

/**
 * @brief VST Plugin types supported
 */
typedef enum {
  VST_PLUGIN_TYPE_UNKNOWN = 0,
  VST_PLUGIN_TYPE_VST2 = 1,
  VST_PLUGIN_TYPE_VST3 = 2
} VSTPluginType;

/**
 * @brief Error codes
 */
typedef enum {
  VST_ERR_OK = 0,
  VST_ERR_FAILED = -1,
  VST_ERR_INVALID_ARG = -2,
  VST_ERR_NOT_INITIALIZED = -3,
  VST_ERR_NOT_SUPPORTED = -4,
  VST_ERR_FILE_NOT_FOUND = -5,
  VST_ERR_OUT_OF_MEMORY = -6
} VSTError;

/**
 * @brief Initialize the VST library
 *
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_initialize(void);

/**
 * @brief Clean up and release resources used by the VST library
 */
void vst_terminate(void);

/**
 * @brief Load a VST plugin from file
 *
 * @param path Path to the VST plugin file (.dll, .so, .vst, .vst3)
 * @param handle Pointer to store the plugin handle
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_load_plugin(const char *path, VSTPluginHandle *handle);

/**
 * @brief Unload a VST plugin and free resources
 *
 * @param handle Handle to the plugin instance
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_unload_plugin(VSTPluginHandle handle);

/**
 * @brief Get the type of the loaded plugin
 *
 * @param handle Handle to the plugin instance
 * @return The plugin type (VST2 or VST3)
 */
VSTPluginType vst_get_plugin_type(VSTPluginHandle handle);

/**
 * @brief Get the name of the loaded plugin
 *
 * @param handle Handle to the plugin instance
 * @param name Buffer to store the name
 * @param size Size of the buffer
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_get_plugin_name(VSTPluginHandle handle, char *name, size_t size);

/**
 * @brief Initialize audio processing for the plugin
 *
 * @param handle Handle to the plugin instance
 * @param setup Audio processing setup parameters
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_setup_processing(VSTPluginHandle handle,
                              const VSTProcessSetup *setup);

/**
 * @brief Process audio through the plugin
 *
 * @param handle Handle to the plugin instance
 * @param inputs Array of input channel buffers
 * @param outputs Array of output channel buffers
 * @param numSamples Number of samples to process
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_process_audio(VSTPluginHandle handle, const float **inputs,
                           float **outputs, uint32_t numSamples);

/**
 * @brief Get the number of parameters for the plugin
 *
 * @param handle Handle to the plugin instance
 * @param count Pointer to store the parameter count
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_get_parameter_count(VSTPluginHandle handle, uint32_t *count);

/**
 * @brief Get information about a parameter
 *
 * @param handle Handle to the plugin instance
 * @param index Parameter index
 * @param info Pointer to store parameter information
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_get_parameter_info(VSTPluginHandle handle, uint32_t index,
                                VSTParameterInfo *info);

/**
 * @brief Get the current value of a parameter
 *
 * @param handle Handle to the plugin instance
 * @param id Parameter ID
 * @param value Pointer to store the parameter value
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_get_parameter_value(VSTPluginHandle handle, uint32_t id,
                                 float *value);

/**
 * @brief Set the value of a parameter
 *
 * @param handle Handle to the plugin instance
 * @param id Parameter ID
 * @param value New parameter value
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_set_parameter_value(VSTPluginHandle handle, uint32_t id,
                                 float value);

/**
 * @brief Set the plugin to bypass mode (if supported)
 *
 * @param handle Handle to the plugin instance
 * @param bypass True to enable bypass, false to disable
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_set_bypass(VSTPluginHandle handle, bool bypass);

/**
 * @brief Check if the plugin supports a specific feature
 *
 * @param handle Handle to the plugin instance
 * @param featureId Feature identifier string
 * @return true if supported, false otherwise
 */
bool vst_supports_feature(VSTPluginHandle handle, const char *featureId);
/**
 * @brief Load a preset from a file
 *
 * @param handle Handle to the plugin instance
 * @param path Path to the preset file (.fxp or .fxb)
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_load_preset(VSTPluginHandle handle, const char *path);
// Add these to your VSTPlugin.h header
VSTError vst_has_editor(VSTPluginHandle handle, int *hasEditor);
VSTError vst_get_editor_size(VSTPluginHandle handle, int *width, int *height);
VSTError vst_open_editor(VSTPluginHandle handle, void *parent);
VSTError vst_close_editor(VSTPluginHandle handle);
VSTError vst_process_editor_idle(VSTPluginHandle handle);

VSTError vst_get_parameter(VSTPluginHandle handle, uint32_t index,
                           float *value);

VSTError vst_set_parameter(VSTPluginHandle handle, uint32_t index, float value);

VSTError vst_get_parameter_name(VSTPluginHandle handle, uint32_t index,
                                char *name, size_t size);

VSTError vst_get_parameter_display(VSTPluginHandle handle, uint32_t index,
                                   char *text, size_t size);
VSTError vst_get_parameter_label(VSTPluginHandle handle, uint32_t index,
                                 char *label, size_t size);

VSTError vst_get_parameter_category(VSTPluginHandle handle, uint32_t index,
                                    char *category, size_t size);

VSTError vst_is_parameter_automatable(VSTPluginHandle handle, uint32_t index,
                                      bool *automatable);

VSTError vst_get_parameter_range(VSTPluginHandle handle, uint32_t index,
                                 float *min_value, float *max_value,
                                 float *default_value);

VSTError vst_get_parameter_step_count(VSTPluginHandle handle, uint32_t index,
                                      int32_t *step_count);

VSTError vst_get_parameter_count(VSTPluginHandle handle, uint32_t *count);
#ifdef __cplusplus
}
#endif

#endif /* YLC_VST_PLUGIN_H */
