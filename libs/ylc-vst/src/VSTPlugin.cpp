#include "VSTPlugin.h"
#include "CocoaBridge.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// VST2 SDK includes
#include <aeffect.h>
#include <aeffectx.h>
// Include these at the top of your VSTPlugin.cpp file
#include <pluginterfaces/base/ftypes.h> // For basic types and enums like kResultOk
#include <pluginterfaces/base/ipluginbase.h> // For IPluginFactory
#include <pluginterfaces/vst/ivstaudioprocessor.h> // For IAudioProcessor, ProcessSetup, ProcessData
#include <pluginterfaces/vst/ivstcomponent.h> // For IComponent
#include <pluginterfaces/vst/ivsteditcontroller.h> // For IEditController, ParameterInfo
#include <pluginterfaces/vst/ivstparameterchanges.h> // For parameter handling
//
namespace vst = Steinberg::Vst;
using namespace Steinberg;

// VST2 constants and typedefs
typedef AEffect *(*VST2EntryProc)(audioMasterCallback);
typedef VstIntPtr (*AudioMasterCallback)(AEffect *effect, VstInt32 opcode,
                                         VstInt32 index, VstIntPtr value,
                                         void *ptr, float opt);

// Generic VST plugin instance structure
struct VSTPluginInstance_t {
  VSTPluginType type;
  void *libraryHandle;
  char pluginPath[512];
  char pluginName[128];
  uint32_t numInputs;
  uint32_t numOutputs;
  uint32_t numParameters;
  bool isInitialized;
  bool isProcessing;
  float sampleRate;
  uint32_t blockSize;

  // VST2 specific
  AEffect *vst2Effect;

  // VST3 specific
  // Using void pointers to avoid direct coupling with VST3 SDK
  void *vst3PlugProvider;
  void *vst3Component;
  void *vst3Processor;
  void *vst3Controller;
  void *vst3ProcessSetup;
  void *vst3ProcessData;
  void *vst3InputBuffers;
  void *vst3OutputBuffers;
};

// VST2 callback functions
static VstIntPtr VSTCALLBACK vst2HostCallback(AEffect *effect, VstInt32 opcode,
                                              VstInt32 index, VstIntPtr value,
                                              void *ptr, float opt) {
  switch (opcode) {
  case audioMasterVersion:
    return 2400; // VST 2.4
  case audioMasterGetSampleRate:
    return effect ? static_cast<VstIntPtr>(
                        ((VSTPluginInstance_t *)effect->user)->sampleRate)
                  : 0;
  case audioMasterGetBlockSize:
    return effect ? static_cast<VstIntPtr>(
                        ((VSTPluginInstance_t *)effect->user)->blockSize)
                  : 0;
  case audioMasterGetCurrentProcessLevel:
    return 0; // Process level is not supported
  case audioMasterGetTime:
    return 0; // Time info is not supported yet
  case audioMasterProcessEvents:
    return 0; // MIDI events are not supported yet
  case audioMasterSizeWindow:
    return 0; // UI is not supported yet
  case audioMasterCanDo:
    return 0; // No canDo's are supported for now
  default:
    return 0;
  }
}

VSTError vst_initialize(void) {
#ifdef __APPLE__
  initCocoaApplication();
#endif

  return VST_ERR_OK;
}

void vst_terminate(void) {}

static VSTError loadVST2Plugin(VSTPluginInstance_t *instance) {
  printf("Trying to load as VST2 plugin...\n");

  // Try the main VST2 entry point names
  VST2EntryProc entryPoint =
      (VST2EntryProc)dlsym(instance->libraryHandle, "VSTPluginMain");
  if (!entryPoint) {
    printf("VSTPluginMain not found, trying main...\n");
    entryPoint = (VST2EntryProc)dlsym(instance->libraryHandle, "main");
  }

  if (!entryPoint) {
    printf("No VST2 entry point found\n");
    return VST_ERR_FAILED;
  }

  printf("Found VST2 entry point, calling it...\n");
  instance->vst2Effect = entryPoint(vst2HostCallback);

  if (!instance->vst2Effect) {
    printf("Entry point returned NULL\n");
    return VST_ERR_FAILED;
  }

  printf("Successfully initialized VST2 plugin\n");

  // Set instance as user data for callbacks
  instance->vst2Effect->user = instance;

  // Get plugin info
  instance->type = VST_PLUGIN_TYPE_VST2;
  instance->numInputs = instance->vst2Effect->numInputs;
  instance->numOutputs = instance->vst2Effect->numOutputs;
  instance->numParameters = instance->vst2Effect->numParams;

  printf("Plugin info: inputs=%d, outputs=%d, parameters=%d\n",
         instance->numInputs, instance->numOutputs, instance->numParameters);

  // Initialize the plugin
  instance->vst2Effect->dispatcher(instance->vst2Effect, effOpen, 0, 0, NULL,
                                   0.0f);

  // Get plugin name
  char tempName[128] = {0};
  instance->vst2Effect->dispatcher(instance->vst2Effect, effGetProductString, 0,
                                   0, tempName, 0.0f);
  strncpy(instance->pluginName, tempName, sizeof(instance->pluginName) - 1);

  printf("Plugin name: %s\n", instance->pluginName);

  instance->isInitialized = true;
  return VST_ERR_OK;
}

// Helper function to load a VST3 plugin
static VSTError loadVST3Plugin(VSTPluginInstance_t *instance) {
  // For VST3, we need to use the VST3 SDK directly
  // This is a stub implementation that returns error
  // In a real implementation, this would use the VST3 SDK
  printf("VST3 plugins are not supported in this version\n");
  return VST_ERR_NOT_SUPPORTED;
}

// Load a VST plugin from file
VSTError vst_load_plugin(const char *path, VSTPluginHandle *handle) {
  if (!path || !handle) {
    return VST_ERR_INVALID_ARG;
  }

  // Create a new instance
  VSTPluginInstance_t *instance =
      (VSTPluginInstance_t *)calloc(1, sizeof(VSTPluginInstance_t));
  if (!instance) {
    return VST_ERR_OUT_OF_MEMORY;
  }

  // Initialize instance
  strncpy(instance->pluginPath, path, sizeof(instance->pluginPath) - 1);
  instance->isInitialized = false;
  instance->isProcessing = false;

  // Open library
  printf("Attempting to load plugin: %s\n", path);
  instance->libraryHandle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!instance->libraryHandle) {
    printf("dlopen error: %s\n", dlerror());
    free(instance);
    return VST_ERR_FILE_NOT_FOUND;
  }
  printf("Successfully opened library\n");

  VSTError result = VST_ERR_FAILED;

  result = loadVST2Plugin(instance);

  if (result != VST_ERR_OK) {
    dlclose(instance->libraryHandle);
    free(instance);
    return result;
  }

  *handle = instance;
  return VST_ERR_OK;
}

// Unload a VST plugin and free resources
VSTError vst_unload_plugin(VSTPluginHandle handle) {
  if (!handle) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    if (instance->vst2Effect) {
      // Close the VST2 plugin
      instance->vst2Effect->dispatcher(instance->vst2Effect, effClose, 0, 0,
                                       NULL, 0.0f);
    }
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 cleanup would go here
  }

  // Close library
  if (instance->libraryHandle) {
    dlclose(instance->libraryHandle);
  }

  // Free instance
  free(instance);

  return VST_ERR_OK;
}

// Get the type of the loaded plugin
VSTPluginType vst_get_plugin_type(VSTPluginHandle handle) {
  if (!handle) {
    return VST_PLUGIN_TYPE_UNKNOWN;
  }

  return ((VSTPluginInstance_t *)handle)->type;
}

// Get the name of the loaded plugin
VSTError vst_get_plugin_name(VSTPluginHandle handle, char *name, size_t size) {
  if (!handle || !name || size == 0) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  strncpy(name, instance->pluginName, size - 1);
  name[size - 1] = '\0';

  return VST_ERR_OK;
}

// Initialize audio processing for the plugin
VSTError vst_setup_processing(VSTPluginHandle handle,
                              const VSTProcessSetup *setup) {
  if (!handle || !setup) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  instance->sampleRate = setup->sampleRate;
  instance->blockSize = setup->maxBlockSize;

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // Set sample rate and block size
    instance->vst2Effect->dispatcher(instance->vst2Effect, effSetSampleRate, 0,
                                     0, NULL, setup->sampleRate);
    instance->vst2Effect->dispatcher(instance->vst2Effect, effSetBlockSize, 0,
                                     setup->maxBlockSize, NULL, 0.0f);

    // Resume (start) the plugin
    instance->vst2Effect->dispatcher(instance->vst2Effect, effMainsChanged, 0,
                                     1, NULL, 0.0f);
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 setup would go here
  }

  instance->isProcessing = true;
  return VST_ERR_OK;
}

// Process audio through the plugin
VSTError vst_process_audio(VSTPluginHandle handle, const float **inputs,
                           float **outputs, uint32_t numSamples) {
  if (!handle || (!inputs && ((VSTPluginInstance_t *)handle)->numInputs > 0) ||
      !outputs || numSamples == 0) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized || !instance->isProcessing) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // Process audio with VST2
    instance->vst2Effect->processReplacing(
        instance->vst2Effect, (float **)inputs, outputs, numSamples);
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 processing would go here
  }

  return VST_ERR_OK;
}

// Get the number of parameters for the plugin
VSTError vst_get_parameter_count(VSTPluginHandle handle, uint32_t *count) {
  if (!handle || !count) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  *count = instance->numParameters;
  return VST_ERR_OK;
}

// Get information about a parameter
VSTError vst_get_parameter_info(VSTPluginHandle handle, uint32_t index,
                                VSTParameterInfo *info) {
  if (!handle || !info) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (index >= instance->numParameters) {
    return VST_ERR_INVALID_ARG;
  }

  // Initialize parameter info with defaults
  info->id = index;
  info->minValue = 0.0f;
  info->maxValue = 1.0f;
  info->defaultValue = 0.0f;

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // Get parameter info for VST2
    char name[128] = {0};
    char label[128] = {0};
    char display[128] = {0};

    instance->vst2Effect->dispatcher(instance->vst2Effect, effGetParamName,
                                     index, 0, name, 0.0f);
    instance->vst2Effect->dispatcher(instance->vst2Effect, effGetParamLabel,
                                     index, 0, label, 0.0f);
    instance->vst2Effect->dispatcher(instance->vst2Effect, effGetParamDisplay,
                                     index, 0, display, 0.0f);

    strncpy(info->name, name, sizeof(info->name) - 1);
    strncpy(info->label, label, sizeof(info->label) - 1);

    // The default range for VST2 parameters is 0.0 to 1.0
    info->defaultValue =
        instance->vst2Effect->getParameter(instance->vst2Effect, index);
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 parameter info would go here
  }

  return VST_ERR_OK;
}

// Get the current value of a parameter
VSTError vst_get_parameter_value(VSTPluginHandle handle, uint32_t id,
                                 float *value) {
  if (!handle || !value) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // In VST2, the ID is the same as the index
    if (id >= instance->numParameters) {
      return VST_ERR_INVALID_ARG;
    }

    *value = instance->vst2Effect->getParameter(instance->vst2Effect, id);
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 parameter value would go here
    return VST_ERR_NOT_SUPPORTED;
  }

  return VST_ERR_OK;
}

// Set the value of a parameter
VSTError vst_set_parameter_value(VSTPluginHandle handle, uint32_t id,
                                 float value) {
  if (!handle) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // In VST2, the ID is the same as the index
    if (id >= instance->numParameters) {
      return VST_ERR_INVALID_ARG;
    }

    instance->vst2Effect->setParameter(instance->vst2Effect, id, value);
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 parameter setting would go here
    return VST_ERR_NOT_SUPPORTED;
  }

  return VST_ERR_OK;
}

// Set the plugin to bypass mode (if supported)
VSTError vst_set_bypass(VSTPluginHandle handle, bool bypass) {
  if (!handle) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // Set bypass parameter (if supported)
    instance->vst2Effect->dispatcher(instance->vst2Effect, effSetBypass, 0,
                                     bypass ? 1 : 0, NULL, 0.0f);
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 bypass would go here
    return VST_ERR_NOT_SUPPORTED;
  }

  return VST_ERR_OK;
}

// Check if the plugin supports a specific feature
bool vst_supports_feature(VSTPluginHandle handle, const char *featureId) {
  if (!handle || !featureId) {
    return false;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return false;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // Check VST2 capabilities
    if (strcmp(featureId, "bypass") == 0) {
      return instance->vst2Effect->dispatcher(instance->vst2Effect, effCanDo, 0,
                                              0, (void *)"bypass", 0.0f) > 0;
    } else if (strcmp(featureId, "midiEvents") == 0) {
      return (instance->vst2Effect->flags & effFlagsIsSynth) ||
             instance->vst2Effect->dispatcher(instance->vst2Effect, effCanDo, 0,
                                              0, (void *)"receiveVstMidiEvent",
                                              0.0f) > 0;
    } else if (strcmp(featureId, "programs") == 0) {
      return instance->vst2Effect->numPrograms > 0;
    }
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 feature check would go here
    return false;
  }

  return false;
}
// VST2 Preset Format Constants
#define CCONST(a, b, c, d)                                                     \
  ((((unsigned int)(a)) << 24) | (((unsigned int)(b)) << 16) |                 \
   (((unsigned int)(c)) << 8) | (((unsigned int)(d))))

// FXP/FXB Common Chunk Identifiers
#define chunkPresetMagic CCONST('C', 'c', 'n', 'K')
#define chunkBankMagic CCONST('C', 'B', 'n', 'k')
#define chunkPresetFile CCONST('F', 'x', 'C', 'k')
#define chunkBankFile CCONST('F', 'x', 'B', 'k')

// Preset Header Structure
typedef struct {
  int32_t chunkMagic; // 'CcnK' or 'CBnk'
  int32_t byteSize;   // Size of this chunk (excluding chunkMagic & byteSize)
  int32_t fxMagic;    // 'FxCk' for presets, 'FxBk' for banks
  int32_t version;    // Format version (currently 1)
  int32_t fxID;       // Plugin unique ID
  int32_t fxVersion;  // Plugin version
  int32_t numParams;  // Number of parameters
  char prgName[28];   // Program name (null-terminated ASCII string)
} VSTPresetHeader;

// Bank Header Structure (extends Preset Header)
typedef struct {
  VSTPresetHeader header;
  int32_t numPrograms;    // Number of programs in bank
  int32_t currentProgram; // Current program index
  char future[124];       // Reserved for future use
} VSTBankHeader;

// Chunk-based preset structure
typedef struct {
  VSTPresetHeader header;
  int32_t size; // Size of the following data
                // Followed by size bytes of chunk data
} VSTChunkPreset;

// Helper function to read a file into memory
static void *readFileIntoMemory(const char *path, size_t *size) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    printf("Failed to open file: %s\n", path);
    return NULL;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  *size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Allocate memory
  void *data = malloc(*size);
  if (!data) {
    fclose(file);
    return NULL;
  }

  // Read file
  size_t readSize = fread(data, 1, *size, file);
  fclose(file);

  if (readSize != *size) {
    free(data);
    return NULL;
  }

  return data;
}

/**
 * Load a preset from a file
 */
VSTError vst_load_preset(VSTPluginHandle handle, const char *path) {
  printf("load preset %s\n", path);
  if (!handle || !path) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  // VST3 is not supported yet
  if (instance->type == VST_PLUGIN_TYPE_VST3) {
    printf("VST3 preset loading is not supported yet\n");
    return VST_ERR_NOT_SUPPORTED;
  }

  // Read the file into memory
  size_t fileSize = 0;
  void *fileData = readFileIntoMemory(path, &fileSize);
  if (!fileData) {
    return VST_ERR_FILE_NOT_FOUND;
  }

  VSTError result = VST_ERR_FAILED;

  // Check if it's a valid preset file
  if (fileSize < sizeof(VSTPresetHeader)) {
    printf("File is too small to be a valid VST preset\n");
    free(fileData);
    return VST_ERR_FAILED;
  }

  VSTPresetHeader *header = (VSTPresetHeader *)fileData;

  // Validate magic values
  if (header->chunkMagic != chunkPresetMagic &&
      header->chunkMagic != chunkBankMagic) {
    printf("Invalid preset file format (bad chunk magic)\n");
    free(fileData);
    return VST_ERR_FAILED;
  }

  // Check for plugin ID match when available
  if (instance->type == VST_PLUGIN_TYPE_VST2 && instance->vst2Effect) {
    int32_t pluginID = instance->vst2Effect->uniqueID;
    if (header->fxID != pluginID) {
      printf(
          "Warning: Preset was created for a different plugin (ID mismatch)\n");
      printf("Expected: %d, Found: %d\n", pluginID, header->fxID);
      // Continue anyway - some plugins can load presets from other plugins
    }
  }

  // Process based on file type
  if (header->fxMagic == chunkPresetFile) {
    // Single preset (FXP)
    printf("Loading FXP preset: %s\n", header->prgName);

    if (instance->type == VST_PLUGIN_TYPE_VST2 && instance->vst2Effect) {
      // Check if it's a chunk-based preset
      if (header->chunkMagic == chunkPresetMagic) {
        // Parameter-based preset
        if (fileSize <
            sizeof(VSTPresetHeader) + (header->numParams * sizeof(float))) {
          printf("Invalid parameter-based preset file (truncated)\n");
          free(fileData);
          return VST_ERR_FAILED;
        }

        // Set parameters one by one
        float *params = (float *)((char *)fileData + sizeof(VSTPresetHeader));
        for (int32_t i = 0;
             i < header->numParams && i < (int32_t)instance->numParameters;
             i++) {
          instance->vst2Effect->setParameter(instance->vst2Effect, i,
                                             params[i]);
        }

        // Set program name
        instance->vst2Effect->dispatcher(instance->vst2Effect,
                                         effSetProgramName, 0, 0,
                                         header->prgName, 0.0f);

        result = VST_ERR_OK;
      } else {
        // Chunk-based preset
        VSTChunkPreset *chunkPreset = (VSTChunkPreset *)fileData;
        if (fileSize < sizeof(VSTChunkPreset) + chunkPreset->size) {
          printf("Invalid chunk-based preset file (truncated)\n");
          free(fileData);
          return VST_ERR_FAILED;
        }

        // Set the chunk
        void *chunkData = (char *)fileData + sizeof(VSTChunkPreset);
        instance->vst2Effect->dispatcher(instance->vst2Effect, effSetChunk, 1,
                                         chunkPreset->size, chunkData, 0.0f);

        result = VST_ERR_OK;
      }
    }
  } else if (header->fxMagic == chunkBankFile) {
    // Bank of presets (FXB)
    printf("Loading FXB bank\n");

    if (instance->type == VST_PLUGIN_TYPE_VST2 && instance->vst2Effect) {
      VSTBankHeader *bankHeader = (VSTBankHeader *)fileData;

      // Check if it's a chunk-based bank
      if (header->chunkMagic == chunkPresetMagic) {
        // Parameter-based bank
        if (fileSize < sizeof(VSTBankHeader) +
                           (bankHeader->numPrograms *
                            (sizeof(VSTPresetHeader) +
                             (bankHeader->header.numParams * sizeof(float))))) {
          printf("Invalid parameter-based bank file (truncated)\n");
          free(fileData);
          return VST_ERR_FAILED;
        }

        // Load programs one by one
        // This is a simplification - in a real implementation, we would need to
        // parse each program properly
        printf("Parameter-based banks are not fully supported yet\n");
        result = VST_ERR_NOT_SUPPORTED;
      } else {
        // Chunk-based bank
        int32_t bankChunkSize =
            *((int32_t *)((char *)fileData + sizeof(VSTBankHeader)));
        void *bankChunkData =
            (char *)fileData + sizeof(VSTBankHeader) + sizeof(int32_t);

        // Set the bank chunk
        instance->vst2Effect->dispatcher(instance->vst2Effect, effSetChunk, 0,
                                         bankChunkSize, bankChunkData, 0.0f);

        // Set current program if specified
        if (bankHeader->currentProgram >= 0 &&
            bankHeader->currentProgram < bankHeader->numPrograms) {
          instance->vst2Effect->dispatcher(instance->vst2Effect, effSetProgram,
                                           0, bankHeader->currentProgram, NULL,
                                           0.0f);
        }

        result = VST_ERR_OK;
      }
    }
  } else {
    printf("Unknown preset format (not FXP or FXB)\n");
    result = VST_ERR_FAILED;
  }

  free(fileData);
  return result;
}

VSTError vst_has_editor(VSTPluginHandle handle, int *hasEditor) {
  if (!handle || !hasEditor) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    *hasEditor = (instance->vst2Effect->flags & effFlagsHasEditor) != 0;
  } else {
    *hasEditor = 0;
  }

  return VST_ERR_OK;
}

VSTError vst_get_editor_size(VSTPluginHandle handle, int *width, int *height) {
  if (!handle || !width || !height) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    ERect *rect = nullptr;
    instance->vst2Effect->dispatcher(instance->vst2Effect, effEditGetRect, 0, 0,
                                     &rect, 0);
    if (rect) {
      *width = rect->right - rect->left;
      *height = rect->bottom - rect->top;
      return VST_ERR_OK;
    }
  }

  return VST_ERR_FAILED;
}

// VSTError vst_open_editor(VSTPluginHandle handle, void *parent) {
//   printf("vst open editor %p win parent: %p\n", handle, parent);
//   if (!handle || !parent) {
//     return VST_ERR_INVALID_ARG;
//   }
//
//   printf("vst open editor %p win parent: %p\n", handle, parent);
//
//   VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;
//
//   if (!instance->isInitialized) {
//     printf("vst open editor %p win parent: %p not init\n", handle, parent);
//     return VST_ERR_NOT_INITIALIZED;
//   }
//
//   if (instance->type == VST_PLUGIN_TYPE_VST2) {
//
//     printf("vst open editor %p win parent: %p vst2\n", handle, parent);
//     if (instance->vst2Effect->flags & effFlagsHasEditor) {
//
//       printf("vst dispatch %p win parent: %p not init\n", handle, parent);
//       instance->vst2Effect->dispatcher(instance->vst2Effect, effEditOpen, 0,
//       0,
//                                        parent, 0);
//       printf("open editor %p\n", instance);
//       return VST_ERR_OK;
//     }
//   }
//
//   printf("open editor %p FAIL\n", instance);
//
//   return VST_ERR_FAILED;
// }
/**
 * Opens the editor GUI for a VST plugin
 * @param plugin The plugin handle
 * @return VST_ERR_OK on success, error code otherwise
 */
VSTError vst_open_editor(VSTPluginHandle plugin) {
  if (!plugin) {
    return VST_ERR_INVALID_PLUGIN;
  }

  VSTPluginInternal *pluginInternal = (VSTPluginInternal *)plugin;

  // Check if the plugin has an editor
  if (!vst_has_editor(plugin)) {
    return VST_ERR_NO_EDITOR;
  }

  // If editor is already open, return success
  if (pluginInternal->editorOpen) {
    return VST_ERR_OK;
  }

  // Handle based on plugin type
  if (pluginInternal->type == VST_PLUGIN_TYPE_VST2) {
    // macOS implementation using our Cocoa bridge
    CocoaWindow *window = createCocoaWindow("VST Plugin GUI", 800, 600);
    if (!window) {
      return VST_ERR_GUI_FAILED;
    }

    pluginInternal->cocoaWindow = window;
    void *nsView = getNSViewFromWindow(window);
  }

  if (pluginInternal->type == VST_PLUGIN_TYPE_VST2) {
    // For VST2
    pluginInternal->vst2->dispatcher(pluginInternal->vst2, effEditOpen, 0, 0,
                                     nsView, 0);

    // Get editor size
    ERect *rect = NULL;
    pluginInternal->vst2->dispatcher(pluginInternal->vst2, effEditGetRect, 0, 0,
                                     &rect, 0);

    if (rect) {
      resizeCocoaWindow(window, rect->right - rect->left,
                        rect->bottom - rect->top);
    }
    // VST2 implementation
    // macOS implementation using Cocoa
    // This is a simplified outline - actual implementation would use
    // Objective-C
  } else {
    return VST_ERR_UNKNOWN_PLUGIN_TYPE;
  }

  // Mark editor as open
  pluginInternal->editorOpen = true;

  return VST_ERR_OK;
}

VSTError vst_close_editor(VSTPluginHandle handle) {
  if (!handle) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    instance->vst2Effect->dispatcher(instance->vst2Effect, effEditClose, 0, 0,
                                     NULL, 0);
    return VST_ERR_OK;
  }

  return VST_ERR_FAILED;
}

VSTError vst_process_editor_idle(VSTPluginHandle handle) {
  if (!handle) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;
  printf("process editor idle %p\n", instance);

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    instance->vst2Effect->dispatcher(instance->vst2Effect, effEditIdle, 0, 0,
                                     NULL, 0);

    printf("process editor idle %p\n", instance);
    return VST_ERR_OK;
  }

  return VST_ERR_FAILED;
}

// Get the normalized value of a parameter (0.0 to 1.0)
VSTError vst_get_parameter(VSTPluginHandle handle, uint32_t index,
                           float *value) {
  if (!handle || !value) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (index >= instance->numParameters) {
    return VST_ERR_INVALID_ARG;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    *value = instance->vst2Effect->getParameter(instance->vst2Effect, index);
    return VST_ERR_OK;
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 parameter handling would go here
    return VST_ERR_NOT_SUPPORTED;
  }

  return VST_ERR_FAILED;
}

// Set the normalized value of a parameter (0.0 to 1.0)
VSTError vst_set_parameter(VSTPluginHandle handle, uint32_t index,
                           float value) {
  if (!handle) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (index >= instance->numParameters) {
    return VST_ERR_INVALID_ARG;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    instance->vst2Effect->setParameter(instance->vst2Effect, index, value);
    return VST_ERR_OK;
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 parameter handling would go here
    return VST_ERR_NOT_SUPPORTED;
  }

  return VST_ERR_FAILED;
}

// Get the name of a parameter
VSTError vst_get_parameter_name(VSTPluginHandle handle, uint32_t index,
                                char *name, size_t size) {
  if (!handle || !name || size == 0) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (index >= instance->numParameters) {
    return VST_ERR_INVALID_ARG;
  }

  // Initialize with empty string
  name[0] = '\0';

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // VST2 uses effGetParamName to get parameter name
    instance->vst2Effect->dispatcher(instance->vst2Effect, effGetParamName,
                                     index, 0, name, 0.0f);

    // Ensure string is null-terminated
    name[size - 1] = '\0';
    return VST_ERR_OK;
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 parameter name would go here
    return VST_ERR_NOT_SUPPORTED;
  }

  return VST_ERR_FAILED;
}

// Get the display value of a parameter (formatted value as string)
VSTError vst_get_parameter_display(VSTPluginHandle handle, uint32_t index,
                                   char *text, size_t size) {
  if (!handle || !text || size == 0) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (index >= instance->numParameters) {
    return VST_ERR_INVALID_ARG;
  }

  // Initialize with empty string
  text[0] = '\0';

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // VST2 uses effGetParamDisplay to get display text
    instance->vst2Effect->dispatcher(instance->vst2Effect, effGetParamDisplay,
                                     index, 0, text, 0.0f);

    // Ensure string is null-terminated
    text[size - 1] = '\0';
    return VST_ERR_OK;
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 parameter display would go here
    return VST_ERR_NOT_SUPPORTED;
  }

  return VST_ERR_FAILED;
}

// Get the label (unit) of a parameter
VSTError vst_get_parameter_label(VSTPluginHandle handle, uint32_t index,
                                 char *label, size_t size) {
  if (!handle || !label || size == 0) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (index >= instance->numParameters) {
    return VST_ERR_INVALID_ARG;
  }

  // Initialize with empty string
  label[0] = '\0';

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // VST2 uses effGetParamLabel to get parameter label
    instance->vst2Effect->dispatcher(instance->vst2Effect, effGetParamLabel,
                                     index, 0, label, 0.0f);

    // Ensure string is null-terminated
    label[size - 1] = '\0';
    return VST_ERR_OK;
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 parameter label would go here
    return VST_ERR_NOT_SUPPORTED;
  }

  return VST_ERR_FAILED;
}

// Get a text description of the parameter category (optional)
VSTError vst_get_parameter_category(VSTPluginHandle handle, uint32_t index,
                                    char *category, size_t size) {
  if (!handle || !category || size == 0) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (index >= instance->numParameters) {
    return VST_ERR_INVALID_ARG;
  }

  // Initialize with empty string
  category[0] = '\0';

  // This is a custom function that may not be supported by all plugins
  // For VST2, we'll return an empty string as categories aren't directly
  // supported
  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // Try to guess parameter category based on name conventions
    char name[64];
    instance->vst2Effect->dispatcher(instance->vst2Effect, effGetParamName,
                                     index, 0, name, 0.0f);

    // Very basic category detection - could be enhanced
    if (strstr(name, "freq") || strstr(name, "hz") || strstr(name, "pitch")) {
      strncpy(category, "Frequency", size - 1);
    } else if (strstr(name, "gain") || strstr(name, "level") ||
               strstr(name, "vol")) {
      strncpy(category, "Level", size - 1);
    } else if (strstr(name, "q") || strstr(name, "res")) {
      strncpy(category, "Q/Resonance", size - 1);
    } else if (strstr(name, "time") || strstr(name, "delay")) {
      strncpy(category, "Time", size - 1);
    } else {
      strncpy(category, "General", size - 1);
    }

    category[size - 1] = '\0';
    return VST_ERR_OK;
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 might support parameter categories
    return VST_ERR_NOT_SUPPORTED;
  }

  return VST_ERR_FAILED;
}

// Check if a parameter is automatable
VSTError vst_is_parameter_automatable(VSTPluginHandle handle, uint32_t index,
                                      bool *automatable) {
  if (!handle || !automatable) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (index >= instance->numParameters) {
    return VST_ERR_INVALID_ARG;
  }

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // In VST2, we can check if a parameter can be automated
    VstIntPtr result = instance->vst2Effect->dispatcher(
        instance->vst2Effect, effCanBeAutomated, index, 0, NULL, 0.0f);

    *automatable = (result > 0);
    return VST_ERR_OK;
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 has different automation flags
    return VST_ERR_NOT_SUPPORTED;
  }

  // Default to true if we can't determine
  *automatable = true;
  return VST_ERR_OK;
}

// Get the min/max/default values for a parameter (if available)
VSTError vst_get_parameter_range(VSTPluginHandle handle, uint32_t index,
                                 float *min_value, float *max_value,
                                 float *default_value) {
  if (!handle) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (index >= instance->numParameters) {
    return VST_ERR_INVALID_ARG;
  }

  // VST2 parameters are normalized to 0.0-1.0 range by default
  if (min_value)
    *min_value = 0.0f;
  if (max_value)
    *max_value = 1.0f;

  // Try to get default value
  if (default_value) {
    if (instance->type == VST_PLUGIN_TYPE_VST2) {
      // VST2 doesn't have a direct way to get default values
      // We could try to use program data or just set a default
      *default_value = 0.0f;

      // Some VST2 plugins support getting default param value via dispatcher
      VstIntPtr result = instance->vst2Effect->dispatcher(
          instance->vst2Effect, effGetParameterProperties, index, 0, NULL,
          0.0f);

      // If not supported, we leave the default at 0.0
    } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
      // VST3 has parameter info structures
      *default_value = 0.0f;
    }
  }

  return VST_ERR_OK;
}

// Helper function for VST parameter stepping
VSTError vst_get_parameter_step_count(VSTPluginHandle handle, uint32_t index,
                                      int32_t *step_count) {
  if (!handle || !step_count) {
    return VST_ERR_INVALID_ARG;
  }

  VSTPluginInstance_t *instance = (VSTPluginInstance_t *)handle;

  if (!instance->isInitialized) {
    return VST_ERR_NOT_INITIALIZED;
  }

  if (index >= instance->numParameters) {
    return VST_ERR_INVALID_ARG;
  }

  // Default to continuous (no steps)
  *step_count = 0;

  if (instance->type == VST_PLUGIN_TYPE_VST2) {
    // VST2 doesn't have a standard way to get step counts
    // We could try to guess based on parameter properties or behavior

    // Some plugins might support this via custom dispatcher calls
    // For now, we'll use a heuristic approach

    // Get the parameter name to check if it might be stepped
    char name[64];
    instance->vst2Effect->dispatcher(instance->vst2Effect, effGetParamName,
                                     index, 0, name, 0.0f);

    // Check if it looks like a switch or enum parameter
    if (strstr(name, "switch") || strstr(name, "mode") ||
        strstr(name, "type") || strstr(name, "enable") || strstr(name, "on") ||
        strstr(name, "off")) {

      // Test with small increments to see if values quantize
      float prev =
          instance->vst2Effect->getParameter(instance->vst2Effect, index);
      float test = 0.0f;
      int detected_steps = 0;
      float last_value = -1.0f;

      for (int i = 0; i <= 100; i++) {
        test = i / 100.0f;
        instance->vst2Effect->setParameter(instance->vst2Effect, index, test);
        float actual =
            instance->vst2Effect->getParameter(instance->vst2Effect, index);

        if (actual != last_value) {
          detected_steps++;
          last_value = actual;
        }
      }

      // Restore the original value
      instance->vst2Effect->setParameter(instance->vst2Effect, index, prev);

      // If we detected just a few distinct values, it's likely stepped
      if (detected_steps <= 10) {
        *step_count = detected_steps - 1;
      }
    }

    return VST_ERR_OK;
  } else if (instance->type == VST_PLUGIN_TYPE_VST3) {
    // VST3 has parameter step information
    return VST_ERR_NOT_SUPPORTED;
  }

  return VST_ERR_FAILED;
}
