
#include "ht.h"
#include "ylc_integration.h"
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// Include YLC headers
#include "common.h"
#include "input.h"
#include "parse.h"
#include "types/inference.h"
#include "types/type.h"
#include "y.tab.h"
void ylc_parse_file_internal(const char *filename, const char *content,
                             time_t version);

void *yy_scan_string(const char *yystr);

// Forward declaration of our initialization function from ylc_stubs.c
void ylc_lsp_init();
void ylc_lsp_init_with_dir(const char *base_dir);

// Range and spatial index utilities
typedef struct {
  int start_line;
  int start_char;
  int end_line;
  int end_char;
} source_range;
// Use your existing AST structure instead of creating our own
typedef struct ast_range_info {
  Ast *node;                        // Your existing AST node
  source_range range;               // Computed range from loc_info
  void *type_info;                  // Type information (from inference)
  struct ast_range_info *parent;    // Parent node (for scope walking)
  struct ast_range_info **children; // Child nodes
  size_t child_count;
  size_t child_capacity;
} ast_range_info;

// Spatial index for fast range queries
typedef struct range_index_node {
  source_range range;
  ast_range_info *ast_info;
  struct range_index_node *left;
  struct range_index_node *right;
} range_index_node;

typedef struct {
  range_index_node *root;
  ast_range_info **all_nodes; // Flat array for iteration
  size_t node_count;
  size_t node_capacity;
} range_index;

// Cached file data structure
typedef struct {
  char *content;                   // File content
  time_t last_modified;            // Last modification time
  Ast *ast;                        // Parsed AST
  TypeEnv *type_env;               // Type environment
  range_index *spatial_index;      // Range-to-node mapping
  struct json_object *diagnostics; // Cached diagnostics
  bool is_parsing;                 // Whether currently being parsed
  bool parse_failed;               // Whether last parse failed
  pthread_mutex_t mutex;           // Mutex for thread safety
} file_cache_entry;

// Worker thread data structure
typedef struct {
  char *filename;
  char *content;
  time_t version;
  bool should_exit;
} parse_request;

typedef struct {
  pthread_t thread;
  pthread_mutex_t queue_mutex;
  pthread_cond_t queue_cond;
  parse_request *requests;
  size_t request_count;
  size_t request_capacity;
  bool should_exit;
  char *assigned_file; // File this worker is responsible for
} file_worker;

// Global state
static ht *file_cache = NULL;   // filename -> file_cache_entry*
static ht *file_workers = NULL; // filename -> file_worker*
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool lsp_initialized = false;

int compare_positions(int line1, int char1, int line2, int char2) {
  if (line1 < line2)
    return -1;
  if (line1 > line2)
    return 1;
  if (char1 < char2)
    return -1;
  if (char1 > char2)
    return 1;
  return 0;
}

bool point_in_range(const source_range *range, int line, int character) {
  return compare_positions(line, character, range->start_line,
                           range->start_char) >= 0 &&
         compare_positions(line, character, range->end_line, range->end_char) <=
             0;
}

bool ranges_overlap(const source_range *a, const source_range *b) {
  return !(compare_positions(a->end_line, a->end_char, b->start_line,
                             b->start_char) < 0 ||
           compare_positions(b->end_line, b->end_char, a->start_line,
                             a->start_char) < 0);
}

// Extract source range from your existing AST node with loc_info
source_range extract_range_from_ast_node(Ast *node) {
  source_range range = {0};

  if (node && node->loc_info) {
    loc_info *loc = (loc_info *)node->loc_info;
    range.start_line = loc->line;
    range.start_char = loc->col;
    range.end_line = loc->line; // Assuming single line for now
    range.end_char = loc->col_end;
  }

  return range;
}

// Create a new AST range info
ast_range_info *create_ast_range_info(Ast *node, source_range range) {
  ast_range_info *info = malloc(sizeof(ast_range_info));
  if (!info)
    return NULL;

  info->node = node;
  info->range = range;
  info->type_info = node->md; // Use existing metadata field for type info
  info->parent = NULL;
  info->children = NULL;
  info->child_count = 0;
  info->child_capacity = 0;

  return info;
}

// Add child to AST range info
void add_child_to_range_info(ast_range_info *parent, ast_range_info *child) {
  if (parent->child_count >= parent->child_capacity) {
    parent->child_capacity =
        parent->child_capacity ? parent->child_capacity * 2 : 4;
    parent->children = realloc(parent->children, sizeof(ast_range_info *) *
                                                     parent->child_capacity);
  }
  parent->children[parent->child_count++] = child;
  child->parent = parent;
}

// Create range index
range_index *create_range_index() {
  range_index *index = malloc(sizeof(range_index));
  if (!index)
    return NULL;

  index->root = NULL;
  index->all_nodes = NULL;
  index->node_count = 0;
  index->node_capacity = 0;

  return index;
}

// Insert into spatial index (binary tree sorted by start position)
range_index_node *insert_into_spatial_index(range_index_node *root,
                                            ast_range_info *info) {
  if (!root) {
    range_index_node *new_node = malloc(sizeof(range_index_node));
    if (!new_node)
      return NULL;

    new_node->range = info->range;
    new_node->ast_info = info;
    new_node->left = NULL;
    new_node->right = NULL;
    return new_node;
  }

  int cmp = compare_positions(info->range.start_line, info->range.start_char,
                              root->range.start_line, root->range.start_char);

  if (cmp <= 0) {
    root->left = insert_into_spatial_index(root->left, info);
  } else {
    root->right = insert_into_spatial_index(root->right, info);
  }

  return root;
}

// Add node to range index
void add_to_range_index(range_index *index, ast_range_info *info) {
  // Add to spatial tree
  index->root = insert_into_spatial_index(index->root, info);

  // Add to flat array for iteration
  if (index->node_count >= index->node_capacity) {
    index->node_capacity =
        index->node_capacity ? index->node_capacity * 2 : 100;
    index->all_nodes = realloc(index->all_nodes,
                               sizeof(ast_range_info *) * index->node_capacity);
  }
  index->all_nodes[index->node_count++] = info;
}

// Find all nodes that contain a point (recursive helper)
void find_nodes_at_position_recursive(range_index_node *node, int line,
                                      int character, ast_range_info ***results,
                                      size_t *count, size_t *capacity) {
  if (!node)
    return;

  // Check if point is in this node's range
  if (point_in_range(&node->range, line, character)) {
    // Expand results array if needed
    if (*count >= *capacity) {
      *capacity = *capacity ? *capacity * 2 : 4;
      *results = realloc(*results, sizeof(ast_range_info *) * (*capacity));
    }
    (*results)[(*count)++] = node->ast_info;
  }

  // Search left subtree if position could be there
  if (compare_positions(line, character, node->range.start_line,
                        node->range.start_char) <= 0) {
    find_nodes_at_position_recursive(node->left, line, character, results,
                                     count, capacity);
  }

  // Search right subtree if position could be there
  if (compare_positions(line, character, node->range.start_line,
                        node->range.start_char) >= 0) {
    find_nodes_at_position_recursive(node->right, line, character, results,
                                     count, capacity);
  }
}

// Find all AST nodes that contain a specific position
ast_range_info **find_nodes_at_position(range_index *index, int line,
                                        int character, size_t *count) {
  ast_range_info **results = NULL;
  size_t capacity = 0;
  *count = 0;

  find_nodes_at_position_recursive(index->root, line, character, &results,
                                   count, &capacity);

  return results;
}

// Find the most specific (smallest) node at a position
ast_range_info *find_most_specific_node_at_position(range_index *index,
                                                    int line, int character) {
  size_t count;
  ast_range_info **nodes =
      find_nodes_at_position(index, line, character, &count);

  if (count == 0) {
    free(nodes);
    return NULL;
  }

  // Find the node with the smallest range (most specific)
  ast_range_info *most_specific = nodes[0];
  for (size_t i = 1; i < count; i++) {
    source_range *current = &nodes[i]->range;
    source_range *best = &most_specific->range;

    // Calculate range sizes (rough approximation)
    int current_size = (current->end_line - current->start_line) * 1000 +
                       (current->end_char - current->start_char);
    int best_size = (best->end_line - best->start_line) * 1000 +
                    (best->end_char - best->start_char);

    if (current_size < best_size) {
      most_specific = nodes[i];
    }
  }

  free(nodes);
  return most_specific;
}

// Recursively build AST range info tree and spatial index
ast_range_info *build_ast_range_info_recursive(Ast *ast, ast_range_info *parent,
                                               range_index *index) {
  if (!ast)
    return NULL;

  source_range range = extract_range_from_ast_node(ast);
  ast_range_info *info = create_ast_range_info(ast, range);
  if (!info)
    return NULL;

  info->parent = parent;

  // Add to spatial index
  add_to_range_index(index, info);

  // Recursively process children based on your AST structure
  switch (ast->tag) {
  case AST_BODY: {
    for (size_t i = 0; i < ast->data.AST_BODY.len; i++) {
      ast_range_info *child = build_ast_range_info_recursive(
          ast->data.AST_BODY.stmts[i], info, index);
      if (child)
        add_child_to_range_info(info, child);
    }
    break;
  }
  case AST_LET: {
    ast_range_info *binding =
        build_ast_range_info_recursive(ast->data.AST_LET.binding, info, index);
    if (binding)
      add_child_to_range_info(info, binding);

    ast_range_info *expr =
        build_ast_range_info_recursive(ast->data.AST_LET.expr, info, index);
    if (expr)
      add_child_to_range_info(info, expr);

    if (ast->data.AST_LET.in_expr) {
      ast_range_info *in_expr = build_ast_range_info_recursive(
          ast->data.AST_LET.in_expr, info, index);
      if (in_expr)
        add_child_to_range_info(info, in_expr);
    }
    break;
  }
  case AST_BINOP: {
    ast_range_info *left =
        build_ast_range_info_recursive(ast->data.AST_BINOP.left, info, index);
    if (left)
      add_child_to_range_info(info, left);

    ast_range_info *right =
        build_ast_range_info_recursive(ast->data.AST_BINOP.right, info, index);
    if (right)
      add_child_to_range_info(info, right);
    break;
  }
  case AST_UNOP: {
    ast_range_info *expr =
        build_ast_range_info_recursive(ast->data.AST_UNOP.expr, info, index);
    if (expr)
      add_child_to_range_info(info, expr);
    break;
  }
  case AST_APPLICATION: {
    ast_range_info *function = build_ast_range_info_recursive(
        ast->data.AST_APPLICATION.function, info, index);
    if (function)
      add_child_to_range_info(info, function);

    ast_range_info *args = build_ast_range_info_recursive(
        ast->data.AST_APPLICATION.args, info, index);
    if (args)
      add_child_to_range_info(info, args);
    break;
  }
  case AST_LAMBDA: {
    ast_range_info *body =
        build_ast_range_info_recursive(ast->data.AST_LAMBDA.body, info, index);
    if (body)
      add_child_to_range_info(info, body);
    // TODO: Handle params, type_annotations, etc.
    break;
  }
  case AST_MATCH: {
    ast_range_info *expr =
        build_ast_range_info_recursive(ast->data.AST_MATCH.expr, info, index);
    if (expr)
      add_child_to_range_info(info, expr);

    ast_range_info *branches = build_ast_range_info_recursive(
        ast->data.AST_MATCH.branches, info, index);
    if (branches)
      add_child_to_range_info(info, branches);
    break;
  }
  case AST_LIST: {
    ast_range_info *items =
        build_ast_range_info_recursive(ast->data.AST_LIST.items, info, index);
    if (items)
      add_child_to_range_info(info, items);
    break;
  }
  case AST_RECORD_ACCESS: {
    ast_range_info *record = build_ast_range_info_recursive(
        ast->data.AST_RECORD_ACCESS.record, info, index);
    if (record)
      add_child_to_range_info(info, record);

    ast_range_info *member = build_ast_range_info_recursive(
        ast->data.AST_RECORD_ACCESS.member, info, index);
    if (member)
      add_child_to_range_info(info, member);
    break;
  }
  case AST_YIELD: {
    ast_range_info *expr =
        build_ast_range_info_recursive(ast->data.AST_YIELD.expr, info, index);
    if (expr)
      add_child_to_range_info(info, expr);
    break;
  }
  case AST_SPREAD_OP: {
    ast_range_info *expr = build_ast_range_info_recursive(
        ast->data.AST_SPREAD_OP.expr, info, index);
    if (expr)
      add_child_to_range_info(info, expr);
    break;
  }
  case AST_RANGE_EXPRESSION: {
    ast_range_info *from = build_ast_range_info_recursive(
        ast->data.AST_RANGE_EXPRESSION.from, info, index);
    if (from)
      add_child_to_range_info(info, from);

    ast_range_info *to = build_ast_range_info_recursive(
        ast->data.AST_RANGE_EXPRESSION.to, info, index);
    if (to)
      add_child_to_range_info(info, to);
    break;
  }
  // Add more cases as needed for other AST node types
  // Leaf nodes (literals, identifiers) don't need special handling
  default:
    break;
  }

  return info;
}

// Build complete range index from AST
range_index *build_range_index_from_ast(Ast *ast) {
  if (!ast)
    return NULL;

  range_index *index = create_range_index();
  if (!index)
    return NULL;

  build_ast_range_info_recursive(ast, NULL, index);

  return index;
}

// Free range index
void free_range_index_node(range_index_node *node) {
  if (!node)
    return;

  free_range_index_node(node->left);
  free_range_index_node(node->right);

  // Free the ast_range_info
  if (node->ast_info) {
    free(node->ast_info->children);
    free(node->ast_info);
  }

  free(node);
}

void free_range_index(range_index *index) {
  if (!index)
    return;

  free_range_index_node(index->root);
  free(index->all_nodes);
  free(index);
}
void ylc_lsp_cache_init() {
  if (lsp_initialized)
    return;

  pthread_mutex_lock(&cache_mutex);
  if (!lsp_initialized) {
    file_cache = ht_create();
    file_workers = ht_create();
    lsp_initialized = true;
  }
  pthread_mutex_unlock(&cache_mutex);
}

// Clean up cache entry
void free_cache_entry(file_cache_entry *entry) {
  if (!entry)
    return;

  pthread_mutex_destroy(&entry->mutex);
  free(entry->content);
  if (entry->ast) {
    // TODO: Add proper AST cleanup function
    // ast_free(entry->ast);
  }
  if (entry->type_env) {
    // TODO: Add proper TypeEnv cleanup function
    // type_env_free(entry->type_env);
  }
  if (entry->spatial_index) {
    free_range_index(entry->spatial_index);
  }
  if (entry->diagnostics) {
    json_object_put(entry->diagnostics);
  }
  free(entry);
}

// Worker thread function
void *file_worker_thread(void *arg) {
  file_worker *worker = (file_worker *)arg;

  while (!worker->should_exit) {
    pthread_mutex_lock(&worker->queue_mutex);

    // Wait for work or exit signal
    while (worker->request_count == 0 && !worker->should_exit) {
      pthread_cond_wait(&worker->queue_cond, &worker->queue_mutex);
    }

    if (worker->should_exit) {
      pthread_mutex_unlock(&worker->queue_mutex);
      break;
    }

    // Get the next request
    parse_request request = worker->requests[0];
    // Shift remaining requests
    for (size_t i = 1; i < worker->request_count; i++) {
      worker->requests[i - 1] = worker->requests[i];
    }
    worker->request_count--;

    pthread_mutex_unlock(&worker->queue_mutex);

    // Process the request
    if (!request.should_exit) {
      ylc_parse_file_internal(request.filename, request.content,
                              request.version);
      free(request.filename);
      free(request.content);
    }
  }

  return NULL;
}

// Create a new worker for a file
file_worker *create_file_worker(const char *filename) {
  file_worker *worker = malloc(sizeof(file_worker));
  if (!worker)
    return NULL;

  worker->assigned_file = strdup(filename);
  worker->should_exit = false;
  worker->request_count = 0;
  worker->request_capacity = 10;
  worker->requests = malloc(sizeof(parse_request) * worker->request_capacity);

  pthread_mutex_init(&worker->queue_mutex, NULL);
  pthread_cond_init(&worker->queue_cond, NULL);

  if (pthread_create(&worker->thread, NULL, file_worker_thread, worker) != 0) {
    free(worker->assigned_file);
    free(worker->requests);
    pthread_mutex_destroy(&worker->queue_mutex);
    pthread_cond_destroy(&worker->queue_cond);
    free(worker);
    return NULL;
  }

  return worker;
}

// Add a parse request to a worker
void worker_add_request(file_worker *worker, const char *filename,
                        const char *content, time_t version) {
  pthread_mutex_lock(&worker->queue_mutex);

  // Expand request array if needed
  if (worker->request_count >= worker->request_capacity) {
    worker->request_capacity *= 2;
    worker->requests = realloc(worker->requests, sizeof(parse_request) *
                                                     worker->request_capacity);
  }

  // Add new request
  parse_request *req = &worker->requests[worker->request_count++];
  req->filename = strdup(filename);
  req->content = strdup(content);
  req->version = version;
  req->should_exit = false;

  pthread_cond_signal(&worker->queue_cond);
  pthread_mutex_unlock(&worker->queue_mutex);
}

// Get or create worker for a file
file_worker *get_file_worker(const char *filename) {
  file_worker *worker = (file_worker *)ht_get(file_workers, filename);
  if (!worker) {
    worker = create_file_worker(filename);
    if (worker) {
      ht_set(file_workers, filename, worker);
    }
  }
  return worker;
}

// Internal parsing function (called by worker thread)
void ylc_parse_file_internal(const char *filename, const char *content,
                             time_t version) {
  pthread_mutex_lock(&cache_mutex);
  file_cache_entry *entry = (file_cache_entry *)ht_get(file_cache, filename);

  if (!entry) {
    entry = malloc(sizeof(file_cache_entry));
    memset(entry, 0, sizeof(file_cache_entry));
    pthread_mutex_init(&entry->mutex, NULL);
    ht_set(file_cache, filename, entry);
  }
  pthread_mutex_unlock(&cache_mutex);

  pthread_mutex_lock(&entry->mutex);

  // Check if we need to reparse
  if (entry->content && entry->last_modified >= version &&
      strcmp(entry->content, content) == 0) {
    pthread_mutex_unlock(&entry->mutex);
    return; // Already up to date
  }

  entry->is_parsing = true;

  // Clean up old data
  free(entry->content);
  if (entry->ast) {
    // TODO: ast_free(entry->ast);
    entry->ast = NULL;
  }
  if (entry->type_env) {
    // TODO: type_env_free(entry->type_env);
    entry->type_env = NULL;
  }
  if (entry->spatial_index) {
    free_range_index(entry->spatial_index);
    entry->spatial_index = NULL;
  }
  if (entry->diagnostics) {
    json_object_put(entry->diagnostics);
    entry->diagnostics = NULL;
  }

  // Update content
  entry->content = strdup(content);
  entry->last_modified = version;
  entry->parse_failed = false;

  pthread_mutex_unlock(&entry->mutex);

  // Perform the actual parsing (outside the lock to allow other operations)
  char *dirname = get_dirname(filename ? filename : ".");
  ylc_lsp_init_with_dir(dirname);

  yylineno = 1;
  yyabsoluteoffset = 0;

  ast_root = Ast_new(AST_BODY);
  ast_root->data.AST_BODY.len = 0;
  ast_root->data.AST_BODY.stmts = malloc(sizeof(Ast *));

  _cur_script = filename;
  _cur_script_content = content;

  yylineno = 1;
  yyabsoluteoffset = 0;
  yy_scan_string(_cur_script_content);

  bool parse_success = (yyparse() == 0);
  Ast *ast = ast_root;

  TypeEnv *env = NULL;
  struct json_object *diagnostics = json_object_new_array();

  if (parse_success) {
    initialize_builtin_types();
    TICtx ti_ctx = {.env = env, .scope = 0};
    ti_ctx.err_stream = stderr; // TODO: Capture errors for diagnostics

    if (!infer(ast, &ti_ctx)) {
      parse_success = false;
      // TODO: Add inference error to diagnostics
    } else if (!solve_program_constraints(ast, &ti_ctx)) {
      parse_success = false;
      // TODO: Add constraint solving error to diagnostics
    }
  }

  // Update cache with results
  pthread_mutex_lock(&entry->mutex);
  entry->ast = parse_success ? ast : NULL;
  entry->type_env = parse_success ? env : NULL;
  entry->spatial_index = parse_success ? build_range_index_from_ast(ast) : NULL;
  entry->diagnostics = diagnostics;
  entry->parse_failed = !parse_success;
  entry->is_parsing = false;
  pthread_mutex_unlock(&entry->mutex);

  free(dirname);
}

// Public API: Request file parsing (async)
void ylc_request_file_parse(const char *filename, const char *content) {
  ylc_lsp_cache_init();

  time_t version = time(NULL);
  file_worker *worker = get_file_worker(filename);
  if (worker) {
    worker_add_request(worker, filename, content, version);
  } else {
    // Fallback to synchronous parsing if worker creation fails
    ylc_parse_file_internal(filename, content, version);
  }
}

// Get AST node at specific position
ast_range_info *ylc_get_node_at_position(const char *filename, int line,
                                         int character) {
  if (!lsp_initialized)
    return NULL;

  pthread_mutex_lock(&cache_mutex);
  file_cache_entry *entry = (file_cache_entry *)ht_get(file_cache, filename);
  pthread_mutex_unlock(&cache_mutex);

  if (!entry)
    return NULL;

  pthread_mutex_lock(&entry->mutex);
  ast_range_info *node = NULL;
  if (entry->spatial_index) {
    node = find_most_specific_node_at_position(entry->spatial_index, line,
                                               character);
  }
  pthread_mutex_unlock(&entry->mutex);

  return node;
}

// Get all nodes at specific position (for overlapping ranges)
ast_range_info **ylc_get_all_nodes_at_position(const char *filename, int line,
                                               int character, size_t *count) {
  if (!lsp_initialized) {
    *count = 0;
    return NULL;
  }

  pthread_mutex_lock(&cache_mutex);
  file_cache_entry *entry = (file_cache_entry *)ht_get(file_cache, filename);
  pthread_mutex_unlock(&cache_mutex);

  if (!entry) {
    *count = 0;
    return NULL;
  }

  pthread_mutex_lock(&entry->mutex);
  ast_range_info **nodes = NULL;
  if (entry->spatial_index) {
    nodes =
        find_nodes_at_position(entry->spatial_index, line, character, count);
  } else {
    *count = 0;
  }
  pthread_mutex_unlock(&entry->mutex);

  return nodes;
}

// Get cached AST for a file (may return NULL if not ready)
Ast *ylc_get_cached_ast(const char *filename) {
  if (!lsp_initialized)
    return NULL;

  pthread_mutex_lock(&cache_mutex);
  file_cache_entry *entry = (file_cache_entry *)ht_get(file_cache, filename);
  pthread_mutex_unlock(&cache_mutex);

  if (!entry)
    return NULL;

  pthread_mutex_lock(&entry->mutex);
  Ast *ast = entry->ast;
  pthread_mutex_unlock(&entry->mutex);

  return ast;
}

// Get cached diagnostics for a file
struct json_object *ylc_get_cached_diagnostics(const char *filename) {
  if (!lsp_initialized)
    return json_object_new_array();

  pthread_mutex_lock(&cache_mutex);
  file_cache_entry *entry = (file_cache_entry *)ht_get(file_cache, filename);
  pthread_mutex_unlock(&cache_mutex);

  if (!entry)
    return json_object_new_array();

  pthread_mutex_lock(&entry->mutex);
  struct json_object *diagnostics = entry->diagnostics;
  if (diagnostics) {
    json_object_get(diagnostics); // Increment reference count
  } else {
    diagnostics = json_object_new_array();
  }
  pthread_mutex_unlock(&entry->mutex);

  return diagnostics;
}

// Check if file is currently being parsed
bool ylc_is_file_parsing(const char *filename) {
  if (!lsp_initialized)
    return false;

  pthread_mutex_lock(&cache_mutex);
  file_cache_entry *entry = (file_cache_entry *)ht_get(file_cache, filename);
  pthread_mutex_unlock(&cache_mutex);

  if (!entry)
    return false;

  pthread_mutex_lock(&entry->mutex);
  bool is_parsing = entry->is_parsing;
  pthread_mutex_unlock(&entry->mutex);

  return is_parsing;
}

// Original utility functions
char *uri_to_filename(const char *uri) {
  if (strncmp(uri, "file://", 7) == 0) {
    return strdup(uri + 7);
  }
  return strdup(uri);
}

struct json_object *create_position(int line, int character) {
  struct json_object *pos = json_object_new_object();
  json_object_object_add(pos, "line", json_object_new_int(line));
  json_object_object_add(pos, "character", json_object_new_int(character));
  return pos;
}

struct json_object *create_range(int start_line, int start_char, int end_line,
                                 int end_char) {
  struct json_object *range = json_object_new_object();
  json_object_object_add(range, "start",
                         create_position(start_line, start_char));
  json_object_object_add(range, "end", create_position(end_line, end_char));
  return range;
}

struct json_object *create_diagnostic(int line, int column, int severity,
                                      const char *message, const char *code) {
  struct json_object *diagnostic = json_object_new_object();

  // LSP diagnostic severity: 1=Error, 2=Warning, 3=Information, 4=Hint
  json_object_object_add(diagnostic, "severity", json_object_new_int(severity));
  json_object_object_add(diagnostic, "message",
                         json_object_new_string(message));

  if (code) {
    json_object_object_add(diagnostic, "code", json_object_new_string(code));
  }

  // Create range for the diagnostic
  json_object_object_add(diagnostic, "range",
                         create_range(line, column, line, column + 1));

  return diagnostic;
}

// Updated API functions that use caching
struct json_object *parse_ylc_document(const char *content,
                                       const char *filename) {
  ylc_request_file_parse(filename, content);

  struct json_object *result = json_object_new_object();
  struct json_object *diagnostics = ylc_get_cached_diagnostics(filename);

  json_object_object_add(result, "diagnostics", diagnostics);
  return result;
}

struct json_object *get_ylc_diagnostics(const char *content,
                                        const char *filename) {
  ylc_request_file_parse(filename, content);
  return ylc_get_cached_diagnostics(filename);
}

// Helper function to find word boundaries at cursor position
char *extract_word_at_position(const char *content, int line, int character) {
  const char *lines = content;
  int current_line = 0;

  // Navigate to the target line
  while (current_line < line && *lines) {
    if (*lines == '\n')
      current_line++;
    lines++;
  }

  if (current_line != line || character < 0)
    return NULL;

  // Navigate to the target character
  const char *pos = lines;
  int current_char = 0;
  while (current_char < character && *pos && *pos != '\n') {
    pos++;
    current_char++;
  }

  if (current_char != character)
    return NULL;

  // Find word boundaries
  const char *start = pos;
  const char *end = pos;

  // Move start backwards to find word start
  while (start > lines && (isalnum(*(start - 1)) || *(start - 1) == '_')) {
    start--;
  }

  // Move end forwards to find word end
  while (*end && (isalnum(*end) || *end == '_')) {
    end++;
  }

  if (start == end)
    return NULL;

  // Extract the word
  size_t len = end - start;
  char *word = malloc(len + 1);
  strncpy(word, start, len);
  word[len] = '\0';

  return word;
}

struct json_object *get_ylc_hover_at_position(const char *content,
                                              const char *filename, int line,
                                              int character) {
  struct json_object *hover = json_object_new_object();

  // Get the most specific AST node at this position
  ast_range_info *node_info =
      ylc_get_node_at_position(filename, line, character);
  if (!node_info) {
    // Request parsing if not available
    ylc_request_file_parse(filename, content);
    return hover; // Return empty hover for now
  }

  // Extract word at position for fallback
  char *word = extract_word_at_position(content, line, character);

  // Build hover information
  struct json_object *contents = json_object_new_object();
  json_object_object_add(contents, "kind", json_object_new_string("markdown"));

  // TODO: Generate meaningful hover content based on AST node type and type
  // info For now, show basic information
  char *hover_text = malloc(1024);
  if (node_info->type_info) {
    // TODO: Format type information
    snprintf(hover_text, 1024, "**%s**\n\nType: `%s`", word ? word : "symbol",
             "unknown"); // Replace with actual type formatting
  } else {
    snprintf(hover_text, 1024, "**%s**\n\nAST Node Type: %d",
             word ? word : "symbol",
             node_info->node ? node_info->node->tag : -1);
  }

  json_object_object_add(contents, "value", json_object_new_string(hover_text));
  json_object_object_add(hover, "contents", contents);

  // Add range information
  json_object_object_add(
      hover, "range",
      create_range(node_info->range.start_line, node_info->range.start_char,
                   node_info->range.end_line, node_info->range.end_char));

  free(hover_text);
  free(word);
  return hover;
}

struct json_object *get_ylc_completions_at_position(const char *content,
                                                    const char *filename,
                                                    int line, int character) {
  struct json_object *completions = json_object_new_array();

  // Get AST node at cursor position to determine scope
  ast_range_info *node_info =
      ylc_get_node_at_position(filename, line, character);
  if (!node_info) {
    // Request parsing if not available
    ylc_request_file_parse(filename, content);
    return completions; // Return empty completions for now
  }

  // TODO: Implement actual completions using YLC symbol table
  // This would involve:
  // 1. Walking up the AST to find the current scope
  // 2. Collecting all visible symbols in that scope
  // 3. Filtering based on partial input at cursor
  // 4. Creating completion items with type information

  // Example of how you might collect symbols from parent scopes:
  ast_range_info *current = node_info;
  while (current) {
    // TODO: If this node defines a scope, collect its symbols
    // For example, if it's a function or block:
    // collect_symbols_from_scope(current->node, completions);
    current = current->parent;
  }

  return completions;
}

// Cleanup function
void ylc_lsp_cache_cleanup() {
  if (!lsp_initialized)
    return;

  pthread_mutex_lock(&cache_mutex);

  // Stop all workers
  if (file_workers) {
    hti it = ht_iterator(file_workers);
    while (ht_next(&it)) {
      file_worker *worker = (file_worker *)it.value;
      worker->should_exit = true;
      pthread_cond_signal(&worker->queue_cond);
      pthread_join(worker->thread, NULL);

      pthread_mutex_destroy(&worker->queue_mutex);
      pthread_cond_destroy(&worker->queue_cond);
      free(worker->assigned_file);
      free(worker->requests);
      free(worker);
    }
    ht_destroy(file_workers);
  }

  // Clean up cache entries
  if (file_cache) {
    hti it = ht_iterator(file_cache);
    while (ht_next(&it)) {
      free_cache_entry((file_cache_entry *)it.value);
    }
    ht_destroy(file_cache);
  }

  lsp_initialized = false;
  pthread_mutex_unlock(&cache_mutex);
}
