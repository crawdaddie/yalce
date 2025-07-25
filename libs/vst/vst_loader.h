#ifndef VST_LOADER_H
#define VST_LOADER_H

#include "lib.h"
#include "node.h"
#include "ylc_datatypes.h"
#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for the VST plugin
typedef void *vst_plugin_handle_t;

// Error codes
typedef enum {
  VST_SUCCESS = 0,
  VST_ERROR_LOAD_FAILED = -1,
  VST_ERROR_INVALID_PLUGIN = -2,
  VST_ERROR_INVALID_HANDLE = -3,
  VST_ERROR_INVALID_PARAMETER = -4,
  VST_ERROR_NOT_LOADED = -5
} vst_error_t;

// Plugin information structure
typedef struct {
  char name[256];
  char vendor[256];
  char product[256];
  int version;
  int num_inputs;
  int num_outputs;
  int num_params;
  int num_programs;
  int unique_id;
} vst_plugin_info_t;

// Parameter information structure
typedef struct {
  char name[256];
  char label[256];
  float value;
  float min_value;
  float max_value;
} vst_param_info_t;

// Library initialization/cleanup
vst_error_t vst_initialize(void);
void vst_cleanup(void);

// Plugin management

NodeRef vst_fx_node(void *handle, NodeRef input);

void *vst_load_plugin_of_arr(_String _plugin_path, _DoubleArray all_params);
void *vst_load_plugin_of_assoc_list(_String _plugin_path, InValList *params);

// void vst_unload_plugin(vst_plugin_handle_t handle);
// vst_error_t vst_is_plugin_loaded(vst_plugin_handle_t handle);
//
// // Plugin information
// vst_error_t vst_get_plugin_info(vst_plugin_handle_t handle,
//                                 vst_plugin_info_t *info);
// vst_error_t vst_get_parameter_info(vst_plugin_handle_t handle, int
// param_index,
//                                    vst_param_info_t *param_info);
//
// // Audio processing
// vst_error_t vst_set_sample_rate(vst_plugin_handle_t handle, float
// sample_rate); vst_error_t vst_set_block_size(vst_plugin_handle_t handle,
// int block_size); vst_error_t vst_process_audio(vst_plugin_handle_t handle,
// float
// **input_buffers,
//                               float **output_buffers, int num_samples);
//
// // Parameter control
// vst_error_t vst_set_parameter(vst_plugin_handle_t handle, int param_index,
//                               float value);
// vst_error_t vst_get_parameter(vst_plugin_handle_t handle, int param_index,
//                               float *value);
//
// // Program/preset management
// vst_error_t vst_set_program(vst_plugin_handle_t handle, int program_index);
// vst_error_t vst_get_program(vst_plugin_handle_t handle, int
// *program_index);
//
// // Utility functions
// const char *vst_error_to_string(vst_error_t error);

#ifdef __cplusplus
}
#endif

#endif // VST_LOADER_H
