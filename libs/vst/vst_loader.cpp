#include "./vst_loader.h"
#include "filter_ext.h"
#include "lib.h"
#include "node.h"
#include "ylc_datatypes.h"
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <map>
#include <mutex>
#include <pluginterfaces/vst2.x/aeffect.h>
#include <pluginterfaces/vst2.x/aeffectx.h>
#include <vector>

VstIntPtr hostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index,
                       VstIntPtr value, void *ptr, float opt) {
  switch (opcode) {
  case 1:        // audioMasterVersion
    return 2400; // VST 2.4
  case 6:        // audioMasterGetTime
    return 0;
  case 7: // audioMasterProcessEvents
    return 0;
  case 13: // audioMasterGetSampleRate
    return 48000;
  case 14: // audioMasterGetBlockSize
    return 512;
  case 15: // audioMasterGetInputLatency
    return 0;
  case 16: // audioMasterGetOutputLatency
    return 0;
  case 23: // audioMasterGetVendorString
    if (ptr)
      strcpy((char *)ptr, "YourHostName");
    return 1;
  case 24: // audioMasterGetProductString
    if (ptr)
      strcpy((char *)ptr, "YourProduct");
    return 1;
  case 25: // audioMasterGetVendorVersion
    return 1000;
  default:
    return 0;
  }
}

typedef AEffect *(*VST_Entry_Proc)(VstIntPtr (*audioMaster)(AEffect *, VstInt32,
                                                            VstInt32, VstIntPtr,
                                                            void *, float));

class VSTPlugin {
private:
  void *libraryHandle;
  AEffect *effect;
  std::vector<float *> inputBuffers;
  std::vector<float *> outputBuffers;
  int blockSize;
  float sampleRate;

public:
  VSTPlugin()
      : libraryHandle(nullptr), effect(nullptr), blockSize(512),
        sampleRate(48000.0f) {}

  ~VSTPlugin() { cleanup(); }

  bool loadPlugin(const char *pluginPath) {
    libraryHandle = dlopen(pluginPath, RTLD_LAZY);
    if (!libraryHandle) {
      std::cerr << "Failed to load plugin: " << dlerror() << std::endl;
      return false;
    }

    VST_Entry_Proc vstEntry =
        (VST_Entry_Proc)dlsym(libraryHandle, "VSTPluginMain");
    if (!vstEntry) {
      vstEntry = (VST_Entry_Proc)dlsym(libraryHandle, "main");
    }

    if (!vstEntry) {
      std::cerr << "Failed to find VST entry point: " << dlerror() << std::endl;
      dlclose(libraryHandle);
      libraryHandle = nullptr;
      return false;
    }

    effect = vstEntry(hostCallback);
    if (!effect || effect->magic != 0x56737450) { // 'VstP'
      std::cerr << "Invalid VST plugin or failed to create effect" << std::endl;
      if (libraryHandle) {
        dlclose(libraryHandle);
        libraryHandle = nullptr;
      }
      return false;
    }

    effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);
    effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, sampleRate);
    effect->dispatcher(effect, effSetBlockSize, 0, blockSize, nullptr, 0.0f);
    effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);

    setupBuffers();

    return true;
  }
  AEffect *getEffect() const { return effect; }

  std::vector<float *> &getInputBuffers() { return inputBuffers; }
  std::vector<float *> &getOutputBuffers() { return outputBuffers; }

  int getBlockSize() const { return blockSize; }
  float getSampleRate() const { return sampleRate; }

  // Update block size if needed to match the audio graph
  bool updateBlockSize(int newBlockSize) {
    if (!effect || newBlockSize <= 0)
      return false;

    if (newBlockSize != blockSize) {
      blockSize = newBlockSize;
      effect->dispatcher(effect, effSetBlockSize, 0, blockSize, nullptr, 0.0f);
      setupBuffers(); // Reallocate buffers with new size
    }
    return true;
  }

  void setupBuffers() {
    if (!effect)
      return;

    for (auto *buffer : inputBuffers) {
      delete[] buffer;
    }
    for (auto *buffer : outputBuffers) {
      delete[] buffer;
    }
    inputBuffers.clear();
    outputBuffers.clear();

    for (int i = 0; i < effect->numInputs; i++) {
      inputBuffers.push_back(new float[blockSize]);
      memset(inputBuffers[i], 0, blockSize * sizeof(float));
    }

    for (int i = 0; i < effect->numOutputs; i++) {
      outputBuffers.push_back(new float[blockSize]);
      memset(outputBuffers[i], 0, blockSize * sizeof(float));
    }
  }

  void processAudio() {
    if (!effect || !effect->processReplacing)
      return;

    effect->processReplacing(effect, inputBuffers.data(), outputBuffers.data(),
                             blockSize);
  }

  void setParameter(int paramIndex, float value) {
    if (effect && paramIndex < effect->numParams) {
      effect->setParameter(effect, paramIndex, value);
    }
  }

  float getParameter(int paramIndex) {
    if (effect && paramIndex < effect->numParams) {
      return effect->getParameter(effect, paramIndex);
    }
    return 0.0f;
  }

  void printPluginInfo() {
    if (!effect) {
      std::cout << "No plugin loaded" << std::endl;
      return;
    }

    char effectName[256] = {0};
    char vendorString[256] = {0};
    char productString[256] = {0};

    effect->dispatcher(effect, effGetEffectName, 0, 0, effectName, 0.0f);
    effect->dispatcher(effect, effGetVendorString, 0, 0, vendorString, 0.0f);
    effect->dispatcher(effect, effGetProductString, 0, 0, productString, 0.0f);

    std::cout << "Plugin Information:" << std::endl;
    std::cout << "Name: " << effectName << std::endl;
    std::cout << "Vendor: " << vendorString << std::endl;
    std::cout << "Product: " << productString << std::endl;
    std::cout << "Version: "
              << effect->dispatcher(effect, effGetVendorVersion, 0, 0, nullptr,
                                    0.0f)
              << std::endl;
    std::cout << "Inputs: " << effect->numInputs << std::endl;
    std::cout << "Outputs: " << effect->numOutputs << std::endl;
    std::cout << "Parameters: " << effect->numParams << std::endl;
    char plabel[256] = {0};
    char pname[256] = {0};
    for (int i = 0; i < effect->numParams; i++) {
      effect->dispatcher(effect, effGetParamName, i, 0, pname, 0.0f);
      float v = effect->getParameter(effect, i);
      std::cout << "  " << i << " : " << pname << " : " << v << std::endl;
    }
    std::cout << "Programs: " << effect->numPrograms << std::endl;
    std::cout << "UniqueID: " << effect->uniqueID << std::endl;
  }

  void cleanup() {
    if (effect) {
      effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
      effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
      effect = nullptr;
    }

    if (libraryHandle) {
      dlclose(libraryHandle);
      libraryHandle = nullptr;
    }

    for (auto *buffer : inputBuffers) {
      delete[] buffer;
    }
    for (auto *buffer : outputBuffers) {
      delete[] buffer;
    }
    inputBuffers.clear();
    outputBuffers.clear();
  }

  bool isLoaded() const { return effect != nullptr; }
};

// C API Implementation
static std::map<vst_plugin_handle_t, VSTPlugin *> g_plugins;
static std::mutex g_plugins_mutex;
static int g_next_handle = 1;

wrap_opaque_ref_in_node_t wrap_opaque = nullptr;
void lookup_wrap_opaque() {
  // RTLD_DEFAULT searches all loaded symbols in the current process
  wrap_opaque =
      (wrap_opaque_ref_in_node_t)dlsym(RTLD_DEFAULT, "wrap_opaque_ref_in_node");

  if (!wrap_opaque) {
    std::cerr << "Failed to find wrap opaque ref function: " << dlerror()
              << std::endl;
    // Handle error - maybe set a flag or use a fallback
  }
}

extern "C" {

void *vst_load_plugin(_String _plugin_path) {
  const char *plugin_path = _plugin_path.chars;
  if (!plugin_path) {
    return nullptr;
  }

  VSTPlugin *plugin = new VSTPlugin();
  if (!plugin->loadPlugin(plugin_path)) {

    delete plugin;
    return nullptr;
  }
  plugin->printPluginInfo();

  return plugin;
}

void *vst_load_plugin_of_arr(_String _plugin_path, _DoubleArray all_params) {
  VSTPlugin *p = (VSTPlugin *)vst_load_plugin(_plugin_path);
  if (!p) {
    return nullptr;
  }
  AEffect *eff = p->getEffect();
  for (int i = 0; i < std::min(eff->numParams, all_params.size); i++) {
    eff->setParameter(eff, i, all_params.data[i]);
  }

  return p;
}

void *vst_load_plugin_of_assoc_list(_String _plugin_path, InValList *params) {
  VSTPlugin *p = (VSTPlugin *)vst_load_plugin(_plugin_path);
  if (!p) {
    return nullptr;
  }
  AEffect *eff = p->getEffect();
  InValList *param = params;
  while (param) {
    if (param->pair.idx < eff->numParams) {
      eff->setParameter(eff, param->pair.idx, (float)param->pair.val);
    }
    param = param->next;
  }

  return p;
}

typedef struct {
  VSTPlugin *handle;
} vst_node_state;

void *vst_fx_node_perform(Node *node, vst_node_state *state, Node *inputs[],
                          int nframes, sample_t _spf) {

  VSTPlugin *plugin = state->handle;

  AEffect *effect = plugin->getEffect();

  if (!effect || !effect->processReplacing) {
    sample_t *out = node->output.buf;
    sample_t *in = inputs[0]->output.buf;
    memcpy(out, in, nframes * node->output.layout * sizeof(sample_t));
    return NULL;
  }

  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;

  std::vector<float *> &vstInputBuffers = plugin->getInputBuffers();
  std::vector<float *> &vstOutputBuffers = plugin->getOutputBuffers();

  int vstInputChannels = effect->numInputs;
  int vstOutputChannels = effect->numOutputs;
  int inputChannels = inputs[0]->output.layout;
  int outputChannels = node->output.layout;
  int vstBlockSize = plugin->getBlockSize();

  int framesProcessed = 0;
  while (framesProcessed < nframes) {
    int framesToProcess = std::min(nframes - framesProcessed, vstBlockSize);

    for (int ch = 0; ch < vstInputChannels; ch++) {
      int sourceChannel =
          std::min(ch, inputChannels - 1); // Handle mono->stereo etc.

      for (int frame = 0; frame < framesToProcess; frame++) {
        int inputIndex =
            (framesProcessed + frame) * inputChannels + sourceChannel;
        vstInputBuffers[ch][frame] = static_cast<float>(in[inputIndex]);
      }

      for (int frame = framesToProcess; frame < vstBlockSize; frame++) {
        vstInputBuffers[ch][frame] = 0.0f;
      }
    }

    effect->processReplacing(effect, vstInputBuffers.data(),
                             vstOutputBuffers.data(), vstBlockSize);

    for (int ch = 0; ch < outputChannels; ch++) {
      int sourceChannel =
          std::min(ch, vstOutputChannels - 1); // Handle stereo->mono etc.

      for (int frame = 0; frame < framesToProcess; frame++) {
        int outputIndex = (framesProcessed + frame) * outputChannels + ch;
        out[outputIndex] =
            static_cast<double>(vstOutputBuffers[sourceChannel][frame]);
      }
    }

    framesProcessed += framesToProcess;
  }

  return NULL;
}

NodeRef vst_fx_node(void *handle, NodeRef input) {
  if (!wrap_opaque) {
    lookup_wrap_opaque();
  }
  int out_chans = ((VSTPlugin *)handle)->getEffect()->numOutputs;
  return wrap_opaque(handle, (void *)vst_fx_node_perform, out_chans, input);
}
}
