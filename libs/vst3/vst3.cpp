#include "./vst3.h"

// VST3 SDK includes
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include <cstring>
#define RELEASE
#include "base/source/fstring.h"


#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;

// Forward declaration of helper function
std::string VST3StringToUtf8(const TChar* vstString);

// Implementation of the opaque handle types
struct VST3_Host_Impl {
  double sample_rate;
  int block_size;
  std::shared_ptr<VST3::Hosting::Module> module;
  std::unique_ptr<PlugProvider> factory;
};

struct VST3_Plugin_Impl {
  VST3_Host_Impl *host;
  IComponent *component;
  IAudioProcessor *processor;
  IEditController *controller;
  ProcessData process_data;
  std::vector<std::vector<float>> input_buffers;
  std::vector<std::vector<float>> output_buffers;
  std::map<ParamID, std::string> paramIdToName;
  std::map<ParamID, std::string> paramIdToUnits;
};

struct VST3_ParamInfo_Impl {
  ParameterInfo info;
};

// Helper class for parameter changes
class SimpleParameterChanges : public IParameterChanges {
public:
  SimpleParameterChanges() {}
  virtual ~SimpleParameterChanges() {}

  // IParameterChanges interface implementation
  int32 PLUGIN_API getParameterCount() override { return 0; }
  IParamValueQueue *PLUGIN_API getParameterData(int32 index) override {
    return nullptr;
  }
  IParamValueQueue *PLUGIN_API addParameterData(const ParamID &id,
                                                int32 &index) override {
    return nullptr;
  }

  DECLARE_FUNKNOWN_METHODS
};

IMPLEMENT_FUNKNOWN_METHODS(SimpleParameterChanges, IParameterChanges,
                           IParameterChanges::iid)

// C API implementation
extern "C" {

VST3_Host *vst3_host_create(double sample_rate, int block_size) {
  VST3_Host_Impl *host = new VST3_Host_Impl();
  if (!host)
    return nullptr;

  host->sample_rate = sample_rate;
  host->block_size = block_size;

  return host;
}

void vst3_host_free(VST3_Host *host) {
  if (host) {
    delete host;
  }
}
#include <dlfcn.h> // For dynamic library loading

// Custom module implementation
struct SimpleModule {
    void* handle;
    IPluginFactory* factory;
    
    SimpleModule() : handle(nullptr), factory(nullptr) {}
    
    bool load(const char* path) {
        // Open the dynamic library
        handle = dlopen(path, RTLD_LOCAL | RTLD_LAZY);
        if (!handle) {
            printf("Failed to load module: %s\n", dlerror());
            return false;
        }
        
        // Get the GetPluginFactory function
        using GetFactoryProc = IPluginFactory* (*)();
        GetFactoryProc getFactory = reinterpret_cast<GetFactoryProc>(dlsym(handle, "GetPluginFactory"));
        
        if (!getFactory) {
            printf("Failed to get factory function: %s\n", dlerror());
            dlclose(handle);
            handle = nullptr;
            return false;
        }
        
        // Call the function to get the factory
        factory = getFactory();
        if (!factory) {
            printf("Failed to create plugin factory\n");
            dlclose(handle);
            handle = nullptr;
            return false;
        }
        
        return true;
    }
    
    ~SimpleModule() {
        if (factory) {
            factory->release();
        }
        if (handle) {
            dlclose(handle);
        }
    }
};
VST3_Plugin *vst3_plugin_load(VST3_Host *host, const char *path) {
  if (!host || !path)
    return nullptr;

  VST3_Plugin_Impl *plugin = new VST3_Plugin_Impl();
  if (!plugin)
    return nullptr;

  plugin->host = host;

  // Load the VST3 module
  std::string error;
   auto module = new SimpleModule();
    if (!module->load(path)) {
        delete module;
        delete plugin;
        return nullptr;
    }
    
    // Store the module in some way (you'd need to modify the host structure)
  // host->module = (VST3::Hosting::Module *)module;
  if (!module) {
    printf("Failed to load VST3 module: %s\n", error.c_str());
    delete plugin;
    return nullptr;
  }

  // Create the plugin provider
  auto factory = host->module->getFactory();
  auto classInfos = factory.classInfos();
  if (classInfos.empty()) {
    printf("No VST3 classes found in the module\n");
    delete plugin;
    return nullptr;
  }

  // Find the first component class
  auto classInfo = std::find_if(classInfos.begin(), classInfos.end(),
                               [](const VST3::Hosting::ClassInfo& info) {
                                 return strcmp(info.category().c_str(), kVstAudioEffectClass) == 0;
                               });
  
  if (classInfo == classInfos.end()) {
    printf("No VST3 component class found in the module\n");
    delete plugin;
    return nullptr;
  }

  // Create plugin provider
  host->factory.reset(new PlugProvider(factory, *classInfo));
  if (!host->factory) {
    delete plugin;
    return nullptr;
  }

  // Create component
  plugin->component = host->factory->getComponent();
  if (!plugin->component) {
    delete plugin;
    return nullptr;
  }

  // Get audio processor interface
  if (plugin->component->queryInterface(IAudioProcessor::iid, (void **)&plugin->processor) != kResultOk) {
    plugin->component->release();
    delete plugin;
    return nullptr;
  }

  // Get edit controller
  plugin->controller = host->factory->getController();
  if (!plugin->controller) {
    plugin->processor->release();
    plugin->component->release();
    delete plugin;
    return nullptr;
  }

  // Initialize the component
  HostApplication hostApp;
  FUnknownPtr<IHostApplication> hostAppPtr(&hostApp);
  if (plugin->component->initialize(hostAppPtr) != kResultOk) {
    plugin->controller->release();
    plugin->processor->release();
    plugin->component->release();
    delete plugin;
    return nullptr;
  }

  // Prepare audio processing setup
  ProcessSetup setup;
  setup.processMode = kRealtime;
  setup.symbolicSampleSize = kSample32;
  setup.maxSamplesPerBlock = host->block_size;
  setup.sampleRate = host->sample_rate;

  if (plugin->processor->setupProcessing(setup) != kResultOk) {
    plugin->component->terminate();
    plugin->controller->release();
    plugin->processor->release();
    plugin->component->release();
    delete plugin;
    return nullptr;
  }

  // Get parameter info
  int32 paramCount = plugin->controller->getParameterCount();
  for (int32 i = 0; i < paramCount; i++) {
    ParameterInfo paramInfo;
    if (plugin->controller->getParameterInfo(i, paramInfo) == kResultOk) {
      String128 paramTitle;
      plugin->controller->getParamStringByValue(
          paramInfo.id, paramInfo.defaultNormalizedValue, paramTitle);
      plugin->paramIdToName[paramInfo.id] = VST3StringToUtf8(paramTitle);

      // There's no direct getParameterUnitByIndex in the SDK, so let's skip this for now
      // or you can add unit info from paramInfo
      plugin->paramIdToUnits[paramInfo.id] = "";
    }
  }

  return plugin;
}

void vst3_plugin_unload(VST3_Plugin *plugin) {
  if (plugin) {
    if (plugin->component) {
      plugin->component->terminate();
      plugin->component->release();
    }
    if (plugin->processor) {
      plugin->processor->release();
    }
    if (plugin->controller) {
      plugin->controller->release();
    }
    delete plugin;
  }
}

VST3_Result vst3_plugin_get_info(VST3_Plugin *plugin, VST3_PluginInfo *info) {
  if (!plugin || !info)
    return VST3_ERROR_INVALID_ARG;

  // Get component info
  // String name(plugin->host->factory->getComponentName());
  // String vendor(plugin->host->factory->getVendorName());
  
  // Handle TUID to unique ID conversion - we'll just use a hash for simplicity
  TUID classId;
  plugin->component->getControllerClassId(classId);
  info->unique_id = static_cast<int>(reinterpret_cast<intptr_t>(classId) & 0xFFFFFFFF);

  // Convert to UTF-8
  // strncpy(info->name, VST3StringToUtf8(name).c_str(), 127);
  // info->name[127] = '\0'; // Ensure null-termination
  
  // strncpy(info->vendor, VST3StringToUtf8(vendor).c_str(), 127);
  // info->vendor[127] = '\0'; // Ensure null-termination

  // Get I/O information
  SpeakerArrangement inArr, outArr;
  tresult inResult = plugin->processor->getBusArrangement(kInput, 0, inArr);
  tresult outResult = plugin->processor->getBusArrangement(kOutput, 0, outArr);
  
  info->num_inputs = (inResult == kResultOk) ? SpeakerArr::getChannelCount(inArr) : 0;
  info->num_outputs = (outResult == kResultOk) ? SpeakerArr::getChannelCount(outArr) : 0;
  info->num_params = plugin->controller->getParameterCount();

  return VST3_OK;
}

VST3_Result vst3_plugin_init(VST3_Plugin *plugin, double sample_rate,
                             int block_size) {
  if (!plugin)
    return VST3_ERROR_INVALID_ARG;

  // Setup processing
  ProcessSetup setup;
  setup.processMode = kRealtime;
  setup.symbolicSampleSize = kSample32;
  setup.maxSamplesPerBlock = block_size;
  setup.sampleRate = sample_rate;

  tresult result = plugin->processor->setupProcessing(setup);
  if (result != kResultOk) {
    return VST3_ERROR_FAILED;
  }

  // Initialize buffers
  VST3_PluginInfo info;
  vst3_plugin_get_info(plugin, &info);

  plugin->input_buffers.resize(info.num_inputs);
  plugin->output_buffers.resize(info.num_outputs);

  for (int i = 0; i < info.num_inputs; i++) {
    plugin->input_buffers[i].resize(block_size);
  }

  for (int i = 0; i < info.num_outputs; i++) {
    plugin->output_buffers[i].resize(block_size);
  }

  // Initialize ProcessData
  plugin->process_data.numSamples = block_size;
  plugin->process_data.processMode = kRealtime;
  plugin->process_data.symbolicSampleSize = kSample32;
  plugin->process_data.numInputs = info.num_inputs > 0 ? 1 : 0;
  plugin->process_data.numOutputs = info.num_outputs > 0 ? 1 : 0;

  return VST3_OK;
}

VST3_Result vst3_plugin_process(VST3_Plugin *plugin, VST3_AudioBuffer *buffer) {
  if (!plugin || !buffer)
    return VST3_ERROR_INVALID_ARG;

  // Setup AudioBusBuffers
  AudioBusBuffers inputBus, outputBus;

  // Setup input buffers
  inputBus.numChannels = buffer->num_input_channels;
  inputBus.silenceFlags = 0;
  std::vector<float *> inputPtrs(buffer->num_input_channels);

  for (int i = 0; i < buffer->num_input_channels; i++) {
    inputPtrs[i] = buffer->inputs[i];
  }

  inputBus.channelBuffers32 = inputPtrs.data();

  // Setup output buffers
  outputBus.numChannels = buffer->num_output_channels;
  outputBus.silenceFlags = 0;
  std::vector<float *> outputPtrs(buffer->num_output_channels);

  for (int i = 0; i < buffer->num_output_channels; i++) {
    outputPtrs[i] = buffer->outputs[i];
  }

  outputBus.channelBuffers32 = outputPtrs.data();

  // Setup processing
  plugin->process_data.inputs =
      buffer->num_input_channels > 0 ? &inputBus : nullptr;
  plugin->process_data.outputs =
      buffer->num_output_channels > 0 ? &outputBus : nullptr;
  plugin->process_data.numSamples = buffer->num_samples;

  // Create empty parameter changes
  SimpleParameterChanges paramChanges;
  plugin->process_data.inputParameterChanges = &paramChanges;
  plugin->process_data.outputParameterChanges = nullptr;

  // Process audio
  tresult result = plugin->processor->process(plugin->process_data);

  return (result == kResultOk) ? VST3_OK : VST3_ERROR_FAILED;
}

int vst3_plugin_get_num_params(VST3_Plugin *plugin) {
  if (!plugin || !plugin->controller)
    return -1;
  return plugin->controller->getParameterCount();
}

VST3_Result vst3_plugin_get_param_info(VST3_Plugin *plugin, int param_index,
                                       VST3_ParamInfo *info) {
  if (!plugin || !info || !plugin->controller)
    return VST3_ERROR_INVALID_ARG;

  if (param_index < 0 ||
      param_index >= plugin->controller->getParameterCount()) {
    return VST3_ERROR_INVALID_ARG;
  }

  ParameterInfo paramInfo;
  tresult result = plugin->controller->getParameterInfo(param_index, paramInfo);

  if (result == kResultOk) {
    // We need a proper way to copy the parameter info to the C struct
    // For now, we'll just copy the necessary parts
    info->id = paramInfo.id;
    info->defaultValue = paramInfo.defaultNormalizedValue;
    info->stepCount = paramInfo.stepCount;
    info->flags = paramInfo.flags;
    
    // Convert parameter title
    strncpy(info->title, VST3StringToUtf8(paramInfo.title).c_str(), 127);
    info->title[127] = '\0'; // Ensure null termination
    
    // Convert units
    strncpy(info->units, VST3StringToUtf8(paramInfo.units).c_str(), 31);
    info->units[31] = '\0'; // Ensure null termination
    
    return VST3_OK;
  }

  return VST3_ERROR_FAILED;
}

VST3_Result vst3_plugin_get_param_value(VST3_Plugin *plugin, int param_id,
                                        VST3_ParamValue *value) {
  if (!plugin || !value || !plugin->controller)
    return VST3_ERROR_INVALID_ARG;

  ParamValue paramValue = plugin->controller->getParamNormalized(param_id);

  value->id = param_id;
  value->value = paramValue;

  // Get the string representation
  String128 valueString;
  plugin->controller->getParamStringByValue(param_id, paramValue, valueString);
  strncpy(value->title, VST3StringToUtf8(valueString).c_str(), 127);
  value->title[127] = '\0'; // Ensure null termination

  // Get units
  auto it = plugin->paramIdToUnits.find(param_id);
  if (it != plugin->paramIdToUnits.end()) {
    strncpy(value->units, it->second.c_str(), 31);
    value->units[31] = '\0'; // Ensure null termination
  } else {
    value->units[0] = '\0';
  }

  return VST3_OK;
}

VST3_Result vst3_plugin_set_param_value(VST3_Plugin *plugin, int param_id,
                                        float value) {
  if (!plugin || !plugin->controller)
    return VST3_ERROR_INVALID_ARG;

  plugin->controller->setParamNormalized(param_id, value);
  return VST3_OK;
}

VST3_Result vst3_plugin_start_processing(VST3_Plugin *plugin) {
  if (!plugin || !plugin->processor)
    return VST3_ERROR_INVALID_ARG;

  tresult result = plugin->processor->setProcessing(true);
  return (result == kResultOk) ? VST3_OK : VST3_ERROR_FAILED;
}

VST3_Result vst3_plugin_stop_processing(VST3_Plugin *plugin) {
  if (!plugin || !plugin->processor)
    return VST3_ERROR_INVALID_ARG;

  tresult result = plugin->processor->setProcessing(false);
  return (result == kResultOk) ? VST3_OK : VST3_ERROR_FAILED;
}

} // extern "C"

// Helper function for string conversion
std::string VST3StringToUtf8(const TChar* vstString) {
  std::basic_string<TChar> tstr(vstString);
  
  // This is a simple conversion that works for ASCII characters
  // For a real application, you would need a proper UTF-16 to UTF-8 conversion
  std::string result;
  for (TChar tc : tstr) {
    if (tc < 128) {
      result += static_cast<char>(tc);
    } else {
      result += '?'; // Replace non-ASCII with '?'
    }
  }
  
  return result;
}
