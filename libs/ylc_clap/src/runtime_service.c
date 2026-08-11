#include "runtime_service.h"

#include "audio_graph.h"
#include "script_jit.h"

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>

struct ylc_runtime_service {
  pthread_mutex_t lock;
  bool initialized;
  uint32_t ref_count;
  uint32_t next_instance_id;
  ylc_script_jit_t *script_jit;
};

static ylc_runtime_service_t g_runtime_service = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .initialized = false,
    .ref_count = 0,
    .next_instance_id = 1,
    .script_jit = NULL,
};

static bool ylc_runtime_service_init_locked(ylc_runtime_service_t *service,
                                            char *error, size_t error_size) {
  if (service->initialized) {
    return true;
  }

  service->script_jit = ylc_script_jit_create(error, error_size);
  if (!service->script_jit) {
    return false;
  }

  ylc_audio_graph_install_node_allocator();

  service->initialized = true;
  if (service->next_instance_id == 0) {
    service->next_instance_id = 1;
  }
  return true;
}

bool ylc_runtime_service_global_init(void) {
  ylc_runtime_service_t *service = &g_runtime_service;
  char error[256] = {0};

  pthread_mutex_lock(&service->lock);
  const bool ok =
      ylc_runtime_service_init_locked(service, error, sizeof(error));
  pthread_mutex_unlock(&service->lock);

  if (!ok) {
    fprintf(stderr, "ylc runtime service init failed: %s\n",
            error[0] ? error : "unknown error");
  }
  return ok;
}

void ylc_runtime_service_global_deinit(void) {
  ylc_runtime_service_t *service = &g_runtime_service;

  pthread_mutex_lock(&service->lock);
  if (service->ref_count == 0) {
    ylc_audio_graph_uninstall_node_allocator();
    ylc_script_jit_destroy(service->script_jit);
    service->script_jit = NULL;
    service->initialized = false;
  }
  pthread_mutex_unlock(&service->lock);
}

ylc_runtime_service_t *ylc_runtime_service_acquire(uint32_t *instance_id) {
  ylc_runtime_service_t *service = &g_runtime_service;

  pthread_mutex_lock(&service->lock);
  if (!service->initialized) {
    char error[256] = {0};
    if (!ylc_runtime_service_init_locked(service, error, sizeof(error))) {
      pthread_mutex_unlock(&service->lock);
      fprintf(stderr, "ylc runtime service acquire failed: %s\n",
              error[0] ? error : "unknown error");
      return NULL;
    }
  }

  const uint32_t id = service->next_instance_id++;
  if (service->next_instance_id == 0) {
    service->next_instance_id = 1;
  }

  service->ref_count++;
  pthread_mutex_unlock(&service->lock);

  if (instance_id) {
    *instance_id = id;
  }
  return service;
}

void ylc_runtime_service_release(ylc_runtime_service_t *service,
                                 uint32_t instance_id) {
  (void)instance_id;

  if (!service) {
    return;
  }

  pthread_mutex_lock(&service->lock);
  if (service->ref_count > 0) {
    service->ref_count--;
  }
  pthread_mutex_unlock(&service->lock);
}

uint32_t ylc_runtime_service_ref_count(const ylc_runtime_service_t *service) {
  if (!service) {
    return 0;
  }

  ylc_runtime_service_t *mutable_service = (ylc_runtime_service_t *)service;
  pthread_mutex_lock(&mutable_service->lock);
  const uint32_t ref_count = mutable_service->ref_count;
  pthread_mutex_unlock(&mutable_service->lock);

  return ref_count;
}

bool ylc_runtime_service_compile_dummy_program(ylc_runtime_service_t *service,
                                               void *plugin_state, char *error,
                                               size_t error_size) {
  if (!service || !plugin_state) {
    if (error && error_size > 0) {
      snprintf(error, error_size, "invalid runtime service compile request");
    }
    return false;
  }

  pthread_mutex_lock(&service->lock);
  if (!service->initialized) {
    if (!ylc_runtime_service_init_locked(service, error, error_size)) {
      pthread_mutex_unlock(&service->lock);
      return false;
    }
  }

  const bool ok = ylc_script_jit_compile_dummy_program(
      service->script_jit, plugin_state, error, error_size);
  pthread_mutex_unlock(&service->lock);
  return ok;
}

bool ylc_runtime_service_compile_script_program(ylc_runtime_service_t *service,
                                                void *plugin_state,
                                                const char *script_path,
                                                char *error,
                                                size_t error_size,
                                                ylc_compile_log_fn log_fn,
                                                void *log_user_data) {
  if (!service || !plugin_state || !script_path || script_path[0] == '\0') {
    if (error && error_size > 0) {
      snprintf(error, error_size,
               "invalid runtime service script compile request");
    }
    return false;
  }

  pthread_mutex_lock(&service->lock);
  if (!service->initialized) {
    if (!ylc_runtime_service_init_locked(service, error, error_size)) {
      pthread_mutex_unlock(&service->lock);
      return false;
    }
  }

  const bool ok = ylc_script_jit_compile_script_program(
      service->script_jit, plugin_state, script_path, error, error_size,
      log_fn, log_user_data);
  pthread_mutex_unlock(&service->lock);
  return ok;
}
