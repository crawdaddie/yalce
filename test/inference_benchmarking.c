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

static void print_header(const char *title) {
  fprintf(stdout, "\n## %s\n", title);
  fprintf(stdout, "----------------------------------------\n");
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
  free(lhs_vars);
  free(rhs_vars);
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
  free(src);
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

  print_header("Full Inference");
  benchmark_full_infer_identity_chain(128);
  benchmark_full_infer_identity_chain(512);

  return 0;
}
