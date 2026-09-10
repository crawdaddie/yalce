#include "../lang/parse.h"
#include "../lang/types/builtins.h"
#include "../lang/types/inference.h"
#include "../lang/types/type.h"
#include "../lang/types/type_ser.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static double ns_to_ms(uint64_t ns) { return (double)ns / 1000000.0; }

static uint64_t rng_state = 0x9e3779b97f4a7c15ULL;

static void rng_seed(uint64_t seed) { rng_state = seed ? seed : 1ULL; }

static uint32_t rng_next_u32(void) {
  rng_state ^= rng_state >> 12;
  rng_state ^= rng_state << 25;
  rng_state ^= rng_state >> 27;
  return (uint32_t)((rng_state * 2685821657736338717ULL) >> 32);
}

static int rng_range(int upper_bound) {
  if (upper_bound <= 0) {
    return 0;
  }
  return (int)(rng_next_u32() % (uint32_t)upper_bound);
}

static void print_header(const char *title) {
  fprintf(stdout, "\n## %s\n", title);
  fprintf(stdout, "----------------------------------------\n");
  fflush(stdout);
}

static void require_type(const char *label, Type *actual, Type *expected) {
  if (!types_equal(actual, expected)) {
    char actual_buf[256] = {0};
    char expected_buf[256] = {0};
    fprintf(stderr, "%s failed\nExpected: %s\nGot: %s\n", label,
            type_to_string(expected, expected_buf),
            type_to_string(actual, actual_buf));
    exit(1);
  }
}

typedef struct {
  Type **items;
  int len;
  int cap;
} TypePool;

static void type_pool_push(TypePool *pool, Type *type) {
  if (pool->len == pool->cap) {
    int next_cap = pool->cap == 0 ? 64 : pool->cap * 2;
    pool->items = realloc(pool->items, sizeof(Type *) * (size_t)next_cap);
    pool->cap = next_cap;
  }
  pool->items[pool->len++] = type;
}

static void benchmark_constraint_chain(int n) {
  TICtx ctx = {0};
  Type **vars = malloc(sizeof(Type *) * (size_t)n);
  for (int i = 0; i < n; i++) {
    vars[i] = next_tvar();
  }
  for (int i = 0; i < n - 1; i++) {
    add_constraint(&ctx, vars[i], vars[i + 1]);
  }
  add_constraint(&ctx, vars[n - 1], &t_int);

  uint64_t started = now_ns();
  Subst *subst = solve_constraints(ctx.constraints);
  uint64_t elapsed = now_ns() - started;

  if (!subst) {
    fprintf(stderr, "constraint_chain(%d) failed to solve\n", n);
    exit(1);
  }
  require_type("constraint_chain root", apply_substitution(subst, vars[0]),
               &t_int);
  fprintf(stdout, "constraint_chain n=%d -> %.3f ms\n", n, ns_to_ms(elapsed));
  fflush(stdout);
  free(vars);
}

static void benchmark_constraint_fanout(int n) {
  TICtx ctx = {0};
  Type *root = next_tvar();
  Type **leaves = malloc(sizeof(Type *) * (size_t)n);
  for (int i = 0; i < n; i++) {
    leaves[i] = next_tvar();
    add_constraint(&ctx, root, leaves[i]);
  }
  add_constraint(&ctx, root, &t_num);

  uint64_t started = now_ns();
  Subst *subst = solve_constraints(ctx.constraints);
  uint64_t elapsed = now_ns() - started;

  if (!subst) {
    fprintf(stderr, "constraint_fanout(%d) failed to solve\n", n);
    exit(1);
  }
  require_type("constraint_fanout leaf", apply_substitution(subst, leaves[0]),
               &t_num);
  fprintf(stdout, "constraint_fanout n=%d -> %.3f ms\n", n, ns_to_ms(elapsed));
  fflush(stdout);
  free(leaves);
}

static Type *make_nested_fn(Type **vars, int depth, Type *result) {
  Type *fn = result;
  for (int i = depth - 1; i >= 0; i--) {
    fn = type_fn(vars[i], fn);
  }
  return fn;
}

static void benchmark_constraint_nested_functions(int depth) {
  TICtx ctx = {0};
  Type **lhs_vars = malloc(sizeof(Type *) * (size_t)depth);
  Type **rhs_vars = malloc(sizeof(Type *) * (size_t)depth);
  for (int i = 0; i < depth; i++) {
    lhs_vars[i] = next_tvar();
    rhs_vars[i] = (i % 2 == 0) ? &t_int : &t_num;
  }

  Type *lhs = make_nested_fn(lhs_vars, depth, next_tvar());
  Type *rhs = make_nested_fn(rhs_vars, depth, &t_bool);
  add_constraint(&ctx, lhs, rhs);

  uint64_t started = now_ns();
  Subst *subst = solve_constraints(ctx.constraints);
  uint64_t elapsed = now_ns() - started;

  if (!subst) {
    fprintf(stderr, "constraint_nested_functions(%d) failed to solve\n", depth);
    exit(1);
  }
  require_type("constraint_nested_functions result",
               apply_substitution(subst, lhs_vars[0]), &t_int);
  fprintf(stdout, "constraint_nested_functions depth=%d -> %.3f ms\n", depth,
          ns_to_ms(elapsed));
  fflush(stdout);
  free(lhs_vars);
  free(rhs_vars);
}

static void benchmark_constraint_shared_spine(int width, int depth) {
  TICtx ctx = {0};
  Type **spine = malloc(sizeof(Type *) * (size_t)(depth + 1));
  for (int i = 0; i <= depth; i++) {
    spine[i] = next_tvar();
  }

  for (int i = 0; i < depth; i++) {
    add_constraint(&ctx, spine[i], spine[i + 1]);
  }
  add_constraint(&ctx, spine[depth], &t_int);

  for (int i = 0; i < width; i++) {
    Type *lhs = spine[depth];
    Type *rhs = &t_int;
    for (int j = depth - 1; j >= 0; j--) {
      lhs = type_fn(spine[j], lhs);
      rhs = type_fn(&t_int, rhs);
    }
    add_constraint(&ctx, lhs, rhs);
  }

  uint64_t started = now_ns();
  Subst *subst = solve_constraints(ctx.constraints);
  uint64_t elapsed = now_ns() - started;

  if (!subst) {
    fprintf(stderr, "constraint_shared_spine(width=%d, depth=%d) failed\n",
            width, depth);
    exit(1);
  }

  require_type("constraint_shared_spine root",
               apply_substitution(subst, spine[0]), &t_int);
  fprintf(stdout, "constraint_shared_spine width=%d depth=%d -> %.3f ms\n",
          width, depth, ns_to_ms(elapsed));
  fflush(stdout);
  free(spine);
}

static void benchmark_constraint_diamond_mesh(int layers, int fanout) {
  TICtx ctx = {0};
  Type *root = next_tvar();
  TypePool current = {0};
  TypePool next = {0};

  type_pool_push(&current, root);
  for (int layer = 0; layer < layers; layer++) {
    next.len = 0;
    for (int i = 0; i < current.len; i++) {
      for (int j = 0; j < fanout; j++) {
        Type *child = next_tvar();
        add_constraint(&ctx, current.items[i], child);
        type_pool_push(&next, child);
      }
    }
    TypePool tmp = current;
    current = next;
    next = tmp;
  }

  for (int i = 0; i < current.len; i++) {
    add_constraint(&ctx, current.items[i], &t_num);
  }

  uint64_t started = now_ns();
  Subst *subst = solve_constraints(ctx.constraints);
  uint64_t elapsed = now_ns() - started;

  if (!subst) {
    fprintf(stderr, "constraint_diamond_mesh(layers=%d, fanout=%d) failed\n",
            layers, fanout);
    exit(1);
  }

  require_type("constraint_diamond_mesh root", apply_substitution(subst, root),
               &t_num);
  fprintf(stdout, "constraint_diamond_mesh layers=%d fanout=%d leaves=%d -> %.3f ms\n",
          layers, fanout, current.len, ns_to_ms(elapsed));
  fflush(stdout);
  free(current.items);
  free(next.items);
}

static char *build_identity_chain_source(int depth) {
  size_t cap = (size_t)depth * 8 + 64;
  char *src = malloc(cap);
  size_t len = 0;

  len += (size_t)snprintf(src + len, cap - len, "let id = fn x -> x;;\n");
  for (int i = 0; i < depth; i++) {
    len += (size_t)snprintf(src + len, cap - len, "id (");
  }
  len += (size_t)snprintf(src + len, cap - len, "1");
  for (int i = 0; i < depth; i++) {
    len += (size_t)snprintf(src + len, cap - len, ")");
  }
  len += (size_t)snprintf(src + len, cap - len, "\n");
  return src;
}

static void benchmark_full_infer_identity_chain(int depth) {
  char *src = build_identity_chain_source(depth);
  Ast *ast = parse_input(src, NULL);
  TICtx ctx = {0};

  uint64_t started = now_ns();
  Type *result = infer(ast, &ctx);
  uint64_t elapsed = now_ns() - started;

  if (!result) {
    fprintf(stderr, "full_infer_identity_chain(%d) failed\n", depth);
    free(src);
    exit(1);
  }
  require_type("full_infer_identity_chain", ast->type, &t_int);
  fprintf(stdout, "full_infer_identity_chain depth=%d -> %.3f ms\n", depth,
          ns_to_ms(elapsed));
  fflush(stdout);
  free(src);
}

static Type *random_concrete_leaf(void) {
  switch (rng_range(3)) {
  case 0:
    return &t_int;
  case 1:
    return &t_num;
  default:
    return &t_bool;
  }
}

static Type *random_pool_leaf(TypePool *vars, TypePool *terms) {
  int choice = rng_range(10);
  if (choice < 6 && vars->len > 0) {
    return vars->items[rng_range(vars->len)];
  }
  if (choice < 9 && terms->len > 0) {
    return terms->items[rng_range(terms->len)];
  }
  return random_concrete_leaf();
}

static Type *build_random_term(TypePool *vars, TypePool *terms, int max_depth) {
  if (max_depth <= 0) {
    return random_pool_leaf(vars, terms);
  }

  int choice = rng_range(100);
  if (choice < 35) {
    return random_pool_leaf(vars, terms);
  }
  if (choice < 55) {
    return type_fn(build_random_term(vars, terms, max_depth - 1),
                   build_random_term(vars, terms, max_depth - 1));
  }
  if (choice < 70) {
    return create_list_type_of_type(build_random_term(vars, terms, max_depth - 1));
  }
  if (choice < 85) {
    return create_array_type(build_random_term(vars, terms, max_depth - 1));
  }

  Type **members = t_alloc(sizeof(Type *) * 3);
  int len = 2 + rng_range(2);
  for (int i = 0; i < len; i++) {
    members[i] = build_random_term(vars, terms, max_depth - 1);
  }
  return create_tuple_type(len, members);
}

static Type *concretize_term_to_int(Type *term) {
  if (!term) {
    return NULL;
  }

  switch (term->kind) {
  case T_VAR:
    return &t_int;
  case T_FN:
    return type_fn(concretize_term_to_int(term->data.T_FN.from),
                   concretize_term_to_int(term->data.T_FN.to));
  case T_CONS:
  case T_SUM: {
    Type **args = NULL;
    if (term->data.T_CONS.num_args > 0) {
      args = t_alloc(sizeof(Type *) * (size_t)term->data.T_CONS.num_args);
      for (int i = 0; i < term->data.T_CONS.num_args; i++) {
        args[i] = concretize_term_to_int(term->data.T_CONS.args[i]);
      }
    }
    Type *copy = t_alloc(sizeof(Type));
    *copy = *term;
    copy->data.T_CONS.args = args;
    return copy;
  }
  default:
    return term;
  }
}

static void benchmark_random_constraint_graph(const char *label, uint64_t seed,
                                              int num_vars, int num_terms,
                                              int num_constraints,
                                              int max_depth) {
  rng_seed(seed);

  TICtx ctx = {0};
  TypePool vars = {0};
  TypePool terms = {0};

  for (int i = 0; i < num_vars; i++) {
    type_pool_push(&vars, next_tvar());
  }

  for (int i = 0; i < num_terms; i++) {
    Type *term = build_random_term(&vars, &terms, max_depth);
    type_pool_push(&terms, term);
  }

  for (int i = 0; i < vars.len; i++) {
    add_constraint(&ctx, vars.items[i], &t_int);
  }

  for (int i = 0; i < num_constraints; i++) {
    Type *left = terms.items[rng_range(terms.len)];
    Type *right = concretize_term_to_int(left);
    add_constraint(&ctx, left, right);
  }

  uint64_t started = now_ns();
  Subst *subst = solve_constraints(ctx.constraints);
  uint64_t elapsed = now_ns() - started;

  if (!subst) {
    fprintf(stderr,
            "%s failed to solve (seed=%llu, vars=%d, terms=%d, constraints=%d)\n",
            label, (unsigned long long)seed, num_vars, num_terms,
            num_constraints);
    exit(1);
  }

  require_type(label, apply_substitution(subst, vars.items[0]), &t_int);
  fprintf(stdout,
          "%s seed=%llu vars=%d terms=%d constraints=%d depth=%d -> %.3f ms\n",
          label, (unsigned long long)seed, num_vars, num_terms,
          num_constraints, max_depth, ns_to_ms(elapsed));
  fflush(stdout);

  free(vars.items);
  free(terms.items);
}

int main(void) {
  reset_type_var_counter();
  initialize_builtin_types();

  print_header("Constraint Solving");
  benchmark_constraint_chain(256);
  benchmark_constraint_chain(1024);
  benchmark_constraint_fanout(256);
  benchmark_constraint_fanout(1024);
  benchmark_constraint_nested_functions(64);
  benchmark_constraint_nested_functions(256);
  benchmark_constraint_shared_spine(128, 64);
  benchmark_constraint_shared_spine(512, 128);
  benchmark_constraint_diamond_mesh(7, 3);
  benchmark_constraint_diamond_mesh(8, 3);
  benchmark_random_constraint_graph("random_constraint_graph_small",
                                    0xC0FFEEULL, 64, 128, 256, 3);
  benchmark_random_constraint_graph("random_constraint_graph_large",
                                    0xC0FFEEULL, 192, 384, 1024, 4);

  print_header("Full Inference");
  benchmark_full_infer_identity_chain(128);
  benchmark_full_infer_identity_chain(512);
  benchmark_full_infer_identity_chain(1024);

  return 0;
}
