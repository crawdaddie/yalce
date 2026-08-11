#ifndef YLC_CLAP_AUDIO_GRAPH_H
#define YLC_CLAP_AUDIO_GRAPH_H

#include "script_runtime.h"

#include "../../engine/node.h"

#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*ylc_audio_graph_is_playing_fn)(void *host_state);
typedef double (*ylc_audio_graph_sample_rate_fn)(void *host_state);

typedef struct ylc_audio_graph_render_op {
  Node *node;
  void *state;
  Node *inputs[MAX_INPUTS];
} ylc_audio_graph_render_op_t;

typedef struct ylc_dummy_audio_graph {
  ylc_program_t program;
  void *host_state;
  ylc_audio_graph_is_playing_fn is_playing;
  ylc_audio_graph_sample_rate_fn sample_rate;
  Node *head;
  Node *tail;
  Node *alloc_head;
  unsigned generation;
  unsigned render_generation;
  ylc_audio_graph_render_op_t render_ops[512];
  uint32_t render_op_count;
} ylc_dummy_audio_graph_t;

void ylc_audio_graph_set_active_graph(ylc_dummy_audio_graph_t *graph);
ylc_dummy_audio_graph_t *ylc_audio_graph_get_active_graph(void);

void ylc_dummy_audio_graph_init(ylc_dummy_audio_graph_t *graph,
                                void *host_state, uint32_t seed,
                                ylc_audio_graph_is_playing_fn is_playing,
                                ylc_audio_graph_sample_rate_fn sample_rate);
ylc_program_t *ylc_dummy_audio_graph_program(ylc_dummy_audio_graph_t *graph);
Node *ylc_audio_graph_play_node(ylc_dummy_audio_graph_t *graph, Node *node);
Node *ylc_audio_graph_reset_node(Node *node);
Node *ylc_audio_graph_play_voice(ylc_dummy_audio_graph_t *graph, Node *node);
Node *ylc_audio_graph_create_scalar_node(double value);
Node *ylc_audio_graph_set_scalar_node(Node *node, double value);
Node *ylc_audio_graph_set_input_scalar(Node *node, int input, double value);
void ylc_audio_graph_clear(ylc_dummy_audio_graph_t *graph);
void ylc_audio_graph_free_all_nodes(ylc_dummy_audio_graph_t *graph);

void ylc_audio_graph_install_node_allocator(void);
void ylc_audio_graph_uninstall_node_allocator(void);

double ylc_read_inlet_node(void *node_raw, int64_t frame);
void node_connect_input(int idx, NodeRef node, NodeRef input);

#ifdef __cplusplus
}
#endif

#endif
