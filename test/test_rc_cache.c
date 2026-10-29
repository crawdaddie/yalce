#include "../lang/llvm/lowering.c"
#include <assert.h>

RTConfig ylc_config = {.profile_lowering = true};

static void test_shape_cache(void) {
  Type text = {.kind = T_STRING};
  Type integer = {.kind = T_INT};
  Type fn = {.kind = T_FN, .data.T_FN = {.from = &integer, .to = &integer},
             .closure_meta = &text};
  Type other = fn;
  TypeEnv env = {0}, other_env = {0};
  TypeHashKey key, copy, different;
  assert(type_hash_resolution(&fn, &key) == TYPE_HASH_OK);
  assert(type_hash_resolution(&other, &copy) == TYPE_HASH_OK);
  rc_shape_put(&key, &env, &fn);
  assert(!key.encoding);
  assert(rc_shape_get(&copy, &env) == &fn);
  assert(rc_shape_get(&copy, &other_env) == NULL);

  // Force a collision: equal hashes must not merge distinct capture layouts.
  other.closure_meta = &integer;
  assert(type_hash_resolution(&other, &different) == TYPE_HASH_OK);
  different.hash = copy.hash;
  assert(rc_shape_get(&different, &env) == NULL);
  rc_shape_put(&different, &env, &other);
  assert(rc_shape_get(&copy, &env) == &fn);
  assert(type_hash_resolution(&other, &different) == TYPE_HASH_OK);
  different.hash = copy.hash;
  assert(rc_shape_get(&different, &env) == &other);
  lower_mir_rc_cache_reset();
  assert(rc_shape_get(&copy, &env) == NULL);
  type_hash_destroy(&copy);
  type_hash_destroy(&different);

  // Rehashing must retain owned encodings and their environment associations.
  enum { SHAPE_COUNT = 4096, NAME_SIZE = 32 };
  char name[NAME_SIZE];
  Type record = {.kind = T_CONS, .data.T_CONS = {.name = name}};
  for (size_t i = 0; i < SHAPE_COUNT; i++) {
    snprintf(name, sizeof(name), "Shape%zu", i);
    assert(type_hash_resolution(&record, &key) == TYPE_HASH_OK);
    rc_shape_put(&key, &env, &text);
    type_hash_destroy(&key);
  }
  for (size_t i = 0; i < SHAPE_COUNT; i++) {
    snprintf(name, sizeof(name), "Shape%zu", i);
    assert(type_hash_resolution(&record, &key) == TYPE_HASH_OK);
    assert(rc_shape_get(&key, &env) == &text);
    type_hash_destroy(&key);
  }
  lower_mir_rc_cache_reset();
}

static void check_alias(Type *target) {
  assert(lower_mir_find_equivalent_rc_alias_type(target) == rc_alias_scan(target));
}

static void test_alias_index(void) {
  Type integer = {.kind = T_INT};
  Type text = {.kind = T_STRING};
  Type *args[] = {&integer};
  Type first = {.kind = T_CONS, .alias = "First",
                .data.T_CONS = {.name = "Tuple", .args = args, .num_args = 1}};
  Type second = first;
  second.alias = "Second";
  Type special = {.kind = T_SUM, .alias = "Special",
                  .data.T_CONS = {.name = "Different"}};
  TypeEnv declaration = {.type = &special};
  Type ref = {.kind = T_RECURSIVE_REF,
              .data.T_RECURSIVE_REF = {.name = "Special", .decl = &declaration},
              .closure_meta = &first};
  Type signature = {.kind = T_FN,
                    .data.T_FN = {.from = &ref, .to = &integer}};
  MirParam params[] = {{.type = &second}};
  MirValue values[] = {{.type = &special}};
  MirInstr instructions[] = {
      {.kind = MIR_CONSTRUCT, .type = &text,
       .data.construct = {.constructor_type = &second}},
      {.kind = MIR_CALL, .data.call = {.callee_type = &special}},
  };
  MirBlock block = {.instrs = {.items = instructions, .len = 2}};
  MirBlock *blocks[] = {&block};
  MirFunction function = {.type = &signature,
                          .params = {.items = params, .len = 1},
                          .values = {.items = values, .len = 1},
                          .blocks = {.items = blocks, .len = 1}};
  MirFunction *functions[] = {&function};
  MirProgram program = {.functions = {.items = functions, .len = 1}};
  lower_mir_current_program = &program;
  lower_mir_rc_cache_reset();

  // All lookup paths agree with the original DFS, including alias shortcuts.
  Type target = first;
  target.alias = NULL;
  check_alias(&target);
  assert(lower_mir_find_equivalent_rc_alias_type(&target) == &first);
  target.alias = "Second";
  check_alias(&target);
  assert(lower_mir_find_equivalent_rc_alias_type(&target) == &first);
  target = special;
  target.kind = T_CONS;
  target.data.T_CONS.name = "Unrelated";
  check_alias(&target);
  assert(lower_mir_find_equivalent_rc_alias_type(&target) == &special);
  target = special;
  target.alias = "First";
  check_alias(&target);
  assert(lower_mir_find_equivalent_rc_alias_type(&target) == &first);
  check_alias(&ref);
  check_alias(&text);
  Type legacy_ref = {.kind = T_VAR, .is_recursive_type_ref = true,
                     .data.T_VAR = {.name = "Special"}};
  check_alias(&legacy_ref);
  legacy_ref.data.T_VAR.name = "Missing";
  check_alias(&legacy_ref);
  assert(lower_mir_find_equivalent_rc_alias_type(&legacy_ref) == NULL);

  // Rebuild with each MIR storage location as the only source of candidates.
  signature.data.T_FN.from = &integer;
  for (size_t source = 0; source < 5; source++) {
    function.params.len = source == 0;
    function.values.len = source == 1;
    function.blocks.len = source >= 2;
    instructions[0].type = source == 2 ? &first : &integer;
    instructions[0].data.construct.constructor_type = source == 3 ? &first : NULL;
    instructions[1].data.call.callee_type = source == 4 ? &first : NULL;
    lower_mir_rc_cache_reset();
    target = first;
    target.alias = NULL;
    check_alias(&target);
    check_alias(&special);
  }

  // Empty programs and reset must never retain aliases from the previous input.
  MirProgram empty = {0};
  lower_mir_current_program = &empty;
  check_alias(&first);
  assert(lower_mir_find_equivalent_rc_alias_type(&first) == NULL);
  lower_mir_rc_cache_reset();
  lower_mir_current_program = NULL;
}

static void test_alias_depth(void) {
  enum { CHAIN_LENGTH = 15 };
  Type chain[CHAIN_LENGTH] = {0};
  Type leaf = {.kind = T_CONS, .alias = "Leaf",
               .data.T_CONS = {.name = "Tuple"}};
  MirFunction function = {0};
  MirFunction *functions[] = {&function};
  MirProgram program = {.functions = {.items = functions, .len = 1}};
  lower_mir_current_program = &program;
  for (size_t i = 0; i < CHAIN_LENGTH; i++) {
    chain[i].kind = T_FN;
    chain[i].data.T_FN.to = i + 1 < CHAIN_LENGTH ? &chain[i + 1] : &leaf;
  }
  for (size_t i = 0; i < CHAIN_LENGTH; i++) {
    function.type = &chain[i];
    lower_mir_rc_cache_reset();
    check_alias(&leaf);
  }
  // Cycles terminate at the same depth boundary as the original search.
  chain[CHAIN_LENGTH - 1].data.T_FN.to = &chain[0];
  lower_mir_rc_cache_reset();
  check_alias(&leaf);
  lower_mir_rc_cache_reset();
  lower_mir_current_program = NULL;
}

int main(void) {
  enum { TYPE_COUNT = 4096 };
  Type *types = calloc(TYPE_COUNT, sizeof(*types));
  assert(types);
  lower_mir_rc_cache_reset();

  // Exceed the old capacity and verify every mapping survives growth.
  for (size_t i = 0; i < TYPE_COUNT; i++) {
    lower_mir_rc_cache_put(&types[i], &types[TYPE_COUNT - i - 1]);
  }
  for (size_t i = 0; i < TYPE_COUNT; i++) {
    assert(lower_mir_rc_cache_get(&types[i]) == &types[TYPE_COUNT - i - 1]);
  }
  assert(lower_rc_profile.full == 0);

  // Replacement and reset must preserve pointer identity semantics.
  lower_mir_rc_cache_put(&types[0], &types[0]);
  assert(lower_mir_rc_cache_get(&types[0]) == &types[0]);
  lower_mir_rc_cache_reset();
  for (size_t i = 0; i < TYPE_COUNT; i++) {
    assert(lower_mir_rc_cache_get(&types[i]) == NULL);
  }
  lower_mir_rc_cache_put(&types[0], &types[1]);
  assert(lower_mir_rc_cache_get(&types[0]) == &types[1]);
  lower_mir_rc_cache_reset();
  free(types);
  test_shape_cache();
  test_alias_index();
  test_alias_depth();
  puts("RC cache growth, replacement, and reset passed");
  return 0;
}
