#include "types/type_hash.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum KeyRelation { KEYS_EQUAL, KEYS_DIFFERENT };

static void compare(const Type *left, const Type *right,
                    enum KeyRelation relation) {
  TypeHashKey a, b;
  assert(type_hash_create(left, &a) == TYPE_HASH_OK);
  assert(type_hash_create(right, &b) == TYPE_HASH_OK);
  if (relation == KEYS_EQUAL) {
    assert(a.hash == b.hash);
    assert(strcmp(a.encoding, b.encoding) == 0);
  } else {
    // These fixtures must differ; production lookups still check collisions.
    assert(a.hash != b.hash);
    assert(strcmp(a.encoding, b.encoding) != 0);
  }
  type_hash_destroy(&a);
  type_hash_destroy(&b);
  assert(!a.encoding && !a.hash);
}

static void test_structure(void) {
  Type integer = {.kind = T_INT};
  Type copy = {.kind = T_INT, .alias = "Number", .scope = 9};
  Type text = {.kind = T_STRING};
  compare(&integer, &copy, KEYS_EQUAL);
  compare(&integer, &text, KEYS_DIFFERENT);

  Type *args[] = {&integer, &text};
  Type *copies[] = {&copy, &text};
  Type tuple = {.kind = T_CONS,
                .data.T_CONS = {.name = "Tuple", .args = args, .num_args = 2}};
  Type other = tuple;
  other.data.T_CONS.args = copies;
  compare(&tuple, &other, KEYS_EQUAL);
  copies[0] = &text;
  copies[1] = &integer;
  compare(&tuple, &other, KEYS_DIFFERENT);
  other = tuple;
  other.kind = T_SUM;
  compare(&tuple, &other, KEYS_DIFFERENT);
  other = tuple;
  other.data.T_CONS.name = "Other";
  compare(&tuple, &other, KEYS_DIFFERENT);

  const char *names[] = {"left", "right"};
  const char *renamed[] = {"right", "left"};
  tuple.data.T_CONS.names = names;
  other = tuple;
  other.data.T_CONS.names = renamed;
  compare(&tuple, &other, KEYS_DIFFERENT);

  // Sanitized symbol spellings must not become structural identities.
  tuple.data.T_CONS.name = "a-b";
  other = tuple;
  other.data.T_CONS.name = "a_b";
  compare(&tuple, &other, KEYS_DIFFERENT);
  tuple.data.T_CONS.name = "";
  other.data.T_CONS.name = NULL;
  compare(&tuple, &other, KEYS_DIFFERENT);
}

static void test_functions(void) {
  Type integer = {.kind = T_INT};
  Type text = {.kind = T_STRING};
  Type fn = {.kind = T_FN, .data.T_FN = {.from = &integer, .to = &text}};
  Type other = fn;
  compare(&fn, &other, KEYS_EQUAL);
  other.data.T_FN.from = &text;
  compare(&fn, &other, KEYS_DIFFERENT);
  other = fn;
  other.data.T_FN.to = &integer;
  compare(&fn, &other, KEYS_DIFFERENT);
  other = fn;
  other.data.T_FN.attributes = FN_ATTR_COROUTINE_CONSTRUCTOR;
  compare(&fn, &other, KEYS_DIFFERENT);
  other = fn;
  other.is_coroutine_instance = true;
  compare(&fn, &other, KEYS_DIFFERENT);
  other = fn;
  other.closure_meta = &integer;
  compare(&fn, &other, KEYS_DIFFERENT);
  fn.closure_meta = &text;
  compare(&fn, &other, KEYS_DIFFERENT);

  Type var = {.kind = T_VAR, .data.T_VAR = {.name = "a", .id = 7}};
  Type renamed = {.kind = T_VAR, .data.T_VAR = {.name = "b", .id = 7}};
  compare(&var, &renamed, KEYS_EQUAL);
  renamed.data.T_VAR.id++;
  compare(&var, &renamed, KEYS_DIFFERENT);
}

static void test_recursion(void) {
  Type left = {.kind = T_CONS, .alias = "Left"};
  Type right = {.kind = T_CONS, .alias = "Right"};
  TypeEnv left_decl = {.type = &left};
  TypeEnv right_decl = {.type = &right};
  Type left_ref = {
      .kind = T_RECURSIVE_REF,
      .data.T_RECURSIVE_REF = {.name = "Left", .decl = &left_decl}};
  Type right_ref = {
      .kind = T_RECURSIVE_REF,
      .data.T_RECURSIVE_REF = {.name = "Right", .decl = &right_decl}};
  Type *left_args[] = {&left_ref};
  Type *right_args[] = {&right_ref};
  left.data.T_CONS.name = right.data.T_CONS.name = "Node";
  left.data.T_CONS.num_args = right.data.T_CONS.num_args = 1;
  left.data.T_CONS.args = left_args;
  right.data.T_CONS.args = right_args;
  compare(&left, &right, KEYS_EQUAL);
  compare(&left, &left_ref, KEYS_EQUAL);

  // Direct cycles and named back-edges encode the same binding.
  right_args[0] = &right;
  compare(&left, &right, KEYS_EQUAL);
  right_ref.data.T_RECURSIVE_REF.decl = NULL;
  right_args[0] = &right_ref;
  compare(&left, &right, KEYS_EQUAL);
  right_ref = (Type){.kind = T_VAR,
                     .is_recursive_type_ref = true,
                     .data.T_VAR = {.name = "Right"}};
  compare(&left, &right, KEYS_EQUAL);

  // Distinguish a nested node's self-reference from a reference to its parent.
  Type inner_a = {.kind = T_CONS};
  Type inner_b = {.kind = T_CONS};
  Type *inner_args_a[] = {&left};
  Type *inner_args_b[] = {&right};
  inner_a.data.T_CONS.name = inner_b.data.T_CONS.name = "Inner";
  inner_a.data.T_CONS.num_args = inner_b.data.T_CONS.num_args = 1;
  inner_a.data.T_CONS.args = inner_args_a;
  inner_b.data.T_CONS.args = inner_args_b;
  left_args[0] = &inner_a;
  right_args[0] = &inner_b;
  compare(&left, &right, KEYS_EQUAL);
  inner_args_b[0] = &inner_b;
  compare(&left, &right, KEYS_DIFFERENT);

  // Declaration-backed mutual recursion must match direct graph edges.
  inner_args_a[0] = &left_ref;
  right_ref = (Type){.kind = T_RECURSIVE_REF,
                     .data.T_RECURSIVE_REF = {.decl = &right_decl}};
  inner_args_b[0] = &right_ref;
  compare(&left, &right, KEYS_EQUAL);
}

static void test_sharing(void) {
  Type integer = {.kind = T_INT};
  Type copied = {.kind = T_INT};
  Type *shared[] = {&integer, &integer};
  Type *separate[] = {&integer, &copied};
  Type a = {.kind = T_CONS,
            .data.T_CONS = {.name = "Tuple", .num_args = 2, .args = shared}};
  Type b = a;
  b.data.T_CONS.args = separate;
  compare(&a, &b, KEYS_EQUAL);

  // A flat constructor must not collide with a nested one.
  Type inner = {.kind = T_CONS,
                .data.T_CONS = {.name = "B", .num_args = 1, .args = shared}};
  Type *nested[] = {&inner};
  a.data.T_CONS.name = "A";
  a.data.T_CONS.num_args = 1;
  a.data.T_CONS.args = nested;
  b.data.T_CONS.name = "A_B";
  b.data.T_CONS.num_args = 1;
  compare(&a, &b, KEYS_DIFFERENT);
}

static void test_errors(void) {
  TypeHashKey key;
  Type module = {.kind = T_MODULE};
  Type ref = {.kind = T_RECURSIVE_REF,
              .data.T_RECURSIVE_REF = {.name = "Missing"}};
  Type var = {.kind = T_VAR, .data.T_VAR = {.id = -1}};
  Type invalid = {.kind = T_CONS, .data.T_CONS = {.num_args = 1}};
  assert(type_hash_create(NULL, &key) == TYPE_HASH_INVALID);
  assert(type_hash_create(&module, &key) == TYPE_HASH_INVALID);
  assert(type_hash_create(&ref, &key) == TYPE_HASH_INVALID);
  assert(type_hash_create(&var, &key) == TYPE_HASH_INVALID);
  assert(type_hash_create(&invalid, &key) == TYPE_HASH_INVALID);
  assert(!key.encoding && !key.hash);
  assert(type_hash_create(&var, NULL) == TYPE_HASH_INVALID);
  type_hash_destroy(NULL);

  TypeEnv self_decl = {.type = &ref};
  ref.data.T_RECURSIVE_REF.decl = &self_decl;
  assert(type_hash_create(&ref, &key) == TYPE_HASH_INVALID);

  // Excessive nesting fails explicitly rather than truncating into a collision.
  enum { DEEP_COUNT = 1024 };
  Type *deep = calloc(DEEP_COUNT, sizeof(*deep));
  assert(deep);
  for (size_t i = 0; i + 1 < DEEP_COUNT; i++) {
    deep[i].kind = T_FN;
    deep[i].data.T_FN.to = &deep[i + 1];
  }
  assert(type_hash_create(deep, &key) == TYPE_HASH_TOO_DEEP);
  assert(!key.encoding && !key.hash);
  free(deep);
}

static void test_resolution_keys(void) {
  Type integer = {.kind = T_INT};
  Type *args[] = {&integer};
  Type a = {.kind = T_CONS,
            .alias = "A",
            .data.T_CONS = {.name = "Tuple", .num_args = 1, .args = args}};
  Type b = a;
  b.alias = "B";
  compare(&a, &b, KEYS_EQUAL);
  TypeHashKey left, right;
  assert(type_hash_resolution(&a, &left) == TYPE_HASH_OK);
  assert(type_hash_resolution(&b, &right) == TYPE_HASH_OK);
  assert(strcmp(left.encoding, right.encoding) != 0);
  assert(left.hash != right.hash);
  type_hash_destroy(&left);
  type_hash_destroy(&right);

  // Nested aliases also affect resolution; unresolved variables bypass caching.
  Type outer = {.kind = T_FN, .data.T_FN = {.from = &a, .to = &integer}};
  assert(type_hash_resolution(&outer, &left) == TYPE_HASH_OK);
  outer.data.T_FN.from = &b;
  assert(type_hash_resolution(&outer, &right) == TYPE_HASH_OK);
  assert(left.hash != right.hash);
  type_hash_destroy(&left);
  type_hash_destroy(&right);
  Type var = {.kind = T_VAR, .data.T_VAR = {.name = "x", .id = 1}};
  outer.data.T_FN.from = &var;
  assert(type_hash_resolution(&outer, &left) == TYPE_HASH_INVALID);
  assert(!left.encoding);
}

int main(void) {
  test_structure();
  test_functions();
  test_recursion();
  test_sharing();
  test_errors();
  test_resolution_keys();
  puts("Type hash tests passed");
  return 0;
}
