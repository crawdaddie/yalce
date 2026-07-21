#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
YLC="$ROOT_DIR/build/ylc"

OWNERSHIP_SRC="$SCRIPT_DIR/mir_scripts/perceus_pipeline.ylc"
CLOSURE_SRC="$SCRIPT_DIR/mir_scripts/closure_partial_pipeline.ylc"
VARIANT_SRC="$SCRIPT_DIR/mir_scripts/variant_match_pipeline.ylc"
CONSTRUCT_SRC="$SCRIPT_DIR/mir_scripts/construct_extract_pipeline.ylc"
ARRAY_OP_SRC="$SCRIPT_DIR/mir_scripts/array_ops_pipeline.ylc"
COROUTINE_SRC="$SCRIPT_DIR/mir_scripts/coroutine_pipeline.ylc"
EXTERN_SRC="$SCRIPT_DIR/mir_scripts/extern_pipeline.ylc"
LLVM_SHAPE_SRC="$SCRIPT_DIR/mir_scripts/llvm_shape_pipeline.ylc"
MATCH_DESTRUCTURE_SRC="$SCRIPT_DIR/mir_scripts/match_destructure_pipeline.ylc"
RECURSIVE_DESTRUCTURE_SRC="$SCRIPT_DIR/mir_scripts/recursive_destructure_pipeline.ylc"
FIB_TEST_SRC="$SCRIPT_DIR/test_scripts/01_fib.ylc"
LIST_MAP_TEST_SRC="$SCRIPT_DIR/test_scripts/04_list_map.ylc"
FIRST_CLASS_TEST_SRC="$SCRIPT_DIR/test_scripts/05_1st_class_fns.ylc"
STD_LISTS_TEST_SRC="$ROOT_DIR/std/Lists.ylc"

if [ ! -x "$YLC" ]; then
  echo "error: $YLC does not exist; run make from the repository root first" >&2
  exit 1
fi

ARTIFACT_DIR="$SCRIPT_DIR/mir_artifacts"
mkdir -p "$ARTIFACT_DIR"

strip_ansi() {
  sed -E $'s/\x1b\\[[0-9;]*[A-Za-z]//g'
}

if [ "${MIR_TEST_COLOR:-1}" = "0" ]; then
  GREEN=''
  RED=''
  BLUE=''
  BOLD=''
  NC=''
else
  GREEN=$'\033[0;32m'
  RED=$'\033[0;31m'
  BLUE=$'\033[0;34m'
  BOLD=$'\033[1m'
  NC=$'\033[0m'
fi

CHECKS=0
PASSES=0
FAILS=0

section() {
  echo
  echo "${BLUE}${BOLD}==>${NC} ${BOLD}$1${NC}"
}

pass() {
  CHECKS=$((CHECKS + 1))
  PASSES=$((PASSES + 1))
  echo "  ${GREEN}✓${NC} $1"
}

fail() {
  CHECKS=$((CHECKS + 1))
  FAILS=$((FAILS + 1))
  echo "  ${RED}✗${NC} $1"
}

assert_contains() {
  local file="$1"
  local pattern="$2"
  local label="$3"
  if ! grep -Eq "$pattern" "$file"; then
    fail "$label"
    return
  fi
  pass "$label"
}

assert_not_contains() {
  local file="$1"
  local pattern="$2"
  local label="$3"
  if grep -Eq "$pattern" "$file"; then
    fail "$label"
    return
  fi
  pass "$label"
}

assert_order() {
  local file="$1"
  local label="$2"
  shift 2

  local last=0
  local pattern
  for pattern in "$@"; do
    local line
    line="$({ grep -nE "$pattern" "$file" || true; } |
      awk -F: -v last="$last" '$1 > last { print $1; exit }')"
    if [ -z "$line" ]; then
      fail "$label: missing ordered pattern '$pattern'"
      return
    fi
    last="$line"
  done
  pass "$label"
}

assert_all_not_contains() {
  local pattern="$1"
  local label="$2"
  shift 2

  local file
  for file in "$@"; do
    if grep -Eq "$pattern" "$file"; then
      fail "$label: found in ${file##*/}"
      return
    fi
  done
  pass "$label"
}

extract_function() {
  local input="$1"
  local name="$2"
  local output="$3"
  awk -v prefix="fn $name(" '
    index($0, prefix) == 1 { printing = 1 }
    printing { print }
    printing && $0 == "}" { exit }
  ' "$input" >"$output"

  if [ ! -s "$output" ]; then
    fail "could not extract MIR function $name"
  fi
}

run_dump_mir() {
  local source="$1"
  local output="$2"
  "$YLC" --dump-mir "$source" 2>&1 | strip_ansi >"$output"
}

run_dump_mir_test() {
  local source="$1"
  local output="$2"
  "$YLC" --test --dump-mir "$source" 2>&1 | strip_ansi >"$output"
}

run_ylc_test() {
  local source="$1"
  local output="$2"
  if "$YLC" --test "$source" >"$output" 2>&1; then
    return 0
  fi

  strip_ansi <"$output" >"$output.stripped"
  mv "$output.stripped" "$output"
  return 1
}

run_dump_llvm_pre() {
  local source="$1"
  local output="$2"
  "$YLC" --dump-ir-pre --verify-ir "$source" 2>&1 | strip_ansi >"$output"
}

run_dump_llvm_pre_test() {
  local source="$1"
  local output="$2"
  "$YLC" --test --dump-ir-pre --verify-ir "$source" 2>&1 | strip_ansi >"$output"
}

OWN_MIR="$ARTIFACT_DIR/perceus_pipeline.mir"
CLOSURE_MIR="$ARTIFACT_DIR/closure_partial_pipeline.mir"
VARIANT_MIR="$ARTIFACT_DIR/variant_match_pipeline.mir"
CONSTRUCT_MIR="$ARTIFACT_DIR/construct_extract_pipeline.mir"
ARRAY_OP_MIR="$ARTIFACT_DIR/array_ops_pipeline.mir"
COROUTINE_MIR="$ARTIFACT_DIR/coroutine_pipeline.mir"
EXTERN_MIR="$ARTIFACT_DIR/extern_pipeline.mir"
LLVM_IR="$ARTIFACT_DIR/perceus_pipeline.ll"
CLOSURE_LLVM_IR="$ARTIFACT_DIR/closure_partial_pipeline.ll"
COROUTINE_LLVM_IR="$ARTIFACT_DIR/coroutine_pipeline.ll"
EXTERN_LLVM_IR="$ARTIFACT_DIR/extern_pipeline.ll"
LLVM_SHAPE_IR="$ARTIFACT_DIR/llvm_shape_pipeline.ll"
MATCH_DESTRUCTURE_MIR="$ARTIFACT_DIR/match_destructure_pipeline.mir"
MATCH_DESTRUCTURE_LLVM_IR="$ARTIFACT_DIR/match_destructure_pipeline.ll"
RECURSIVE_DESTRUCTURE_MIR="$ARTIFACT_DIR/recursive_destructure_pipeline.mir"
RECURSIVE_DESTRUCTURE_LLVM_IR="$ARTIFACT_DIR/recursive_destructure_pipeline.ll"
FIB_TEST_MIR="$ARTIFACT_DIR/01_fib.test.mir"
FIB_TEST_OUT="$ARTIFACT_DIR/01_fib.test.out"
LIST_MAP_TEST_LLVM_IR="$ARTIFACT_DIR/04_list_map.test.ll"
LIST_MAP_TEST_OUT="$ARTIFACT_DIR/04_list_map.test.out"
FIRST_CLASS_TEST_MIR="$ARTIFACT_DIR/05_1st_class_fns.test.mir"
FIRST_CLASS_TEST_LLVM_IR="$ARTIFACT_DIR/05_1st_class_fns.test.ll"
FIRST_CLASS_TEST_OUT="$ARTIFACT_DIR/05_1st_class_fns.test.out"
STD_LISTS_TEST_MIR="$ARTIFACT_DIR/std_Lists.test.mir"
STD_LISTS_TEST_LLVM_IR="$ARTIFACT_DIR/std_Lists.test.ll"
STD_LISTS_TEST_OUT="$ARTIFACT_DIR/std_Lists.test.out"

run_dump_mir "$OWNERSHIP_SRC" "$OWN_MIR"
run_dump_mir "$CLOSURE_SRC" "$CLOSURE_MIR"
run_dump_mir "$VARIANT_SRC" "$VARIANT_MIR"
run_dump_mir "$CONSTRUCT_SRC" "$CONSTRUCT_MIR"
run_dump_mir "$ARRAY_OP_SRC" "$ARRAY_OP_MIR"
run_dump_mir "$COROUTINE_SRC" "$COROUTINE_MIR"
run_dump_mir "$EXTERN_SRC" "$EXTERN_MIR"
run_dump_mir "$MATCH_DESTRUCTURE_SRC" "$MATCH_DESTRUCTURE_MIR"
run_dump_mir "$RECURSIVE_DESTRUCTURE_SRC" "$RECURSIVE_DESTRUCTURE_MIR"
run_dump_llvm_pre "$OWNERSHIP_SRC" "$LLVM_IR"
run_dump_llvm_pre "$CLOSURE_SRC" "$CLOSURE_LLVM_IR"
run_dump_llvm_pre "$COROUTINE_SRC" "$COROUTINE_LLVM_IR"
run_dump_llvm_pre "$EXTERN_SRC" "$EXTERN_LLVM_IR"
run_dump_llvm_pre "$LLVM_SHAPE_SRC" "$LLVM_SHAPE_IR"
run_dump_llvm_pre "$MATCH_DESTRUCTURE_SRC" "$MATCH_DESTRUCTURE_LLVM_IR"
run_dump_llvm_pre "$RECURSIVE_DESTRUCTURE_SRC" "$RECURSIVE_DESTRUCTURE_LLVM_IR"
run_dump_mir_test "$FIB_TEST_SRC" "$FIB_TEST_MIR"
run_dump_mir_test "$FIRST_CLASS_TEST_SRC" "$FIRST_CLASS_TEST_MIR"
run_dump_mir_test "$STD_LISTS_TEST_SRC" "$STD_LISTS_TEST_MIR"
true; # skip list_map llvm
run_dump_llvm_pre_test "$FIRST_CLASS_TEST_SRC" "$FIRST_CLASS_TEST_LLVM_IR"
true; # skip std_lists llvm

FIB_TEST_STATUS=0
run_ylc_test "$FIB_TEST_SRC" "$FIB_TEST_OUT" || FIB_TEST_STATUS=$?
LIST_MAP_TEST_STATUS=0
run_ylc_test "$LIST_MAP_TEST_SRC" "$LIST_MAP_TEST_OUT" || LIST_MAP_TEST_STATUS=$?
FIRST_CLASS_TEST_STATUS=0
run_ylc_test "$FIRST_CLASS_TEST_SRC" "$FIRST_CLASS_TEST_OUT" || FIRST_CLASS_TEST_STATUS=$?
STD_LISTS_TEST_STATUS=0
run_ylc_test "$STD_LISTS_TEST_SRC" "$STD_LISTS_TEST_OUT" || STD_LISTS_TEST_STATUS=$?

DUP_BODY="$ARTIFACT_DIR/duplicated_array.mir"
MOVED_BODY="$ARTIFACT_DIR/moved_call_arg.mir"
RETURNED_BODY="$ARTIFACT_DIR/returned_array.mir"
BORROWED_BODY="$ARTIFACT_DIR/borrowed_array_size.mir"
BRANCH_BODY="$ARTIFACT_DIR/branch_drop_returned_array.mir"
BRANCH_CONSUME_BODY="$ARTIFACT_DIR/branch_consume_array.mir"
BRANCH_RETURN_BODY="$ARTIFACT_DIR/branch_return_array.mir"
PHI_CONSUME_BODY="$ARTIFACT_DIR/phi_select_preexisting_array_consume.mir"
PHI_DUP_BODY="$ARTIFACT_DIR/phi_select_preexisting_array_duplicate.mir"
PHI_BORROW_RETURN_BODY="$ARTIFACT_DIR/phi_select_preexisting_array_borrow_return.mir"
MATCH_BODY="$ARTIFACT_DIR/match_unused_heap_result.mir"
EXTRACTED_BODY="$ARTIFACT_DIR/extracted_list.mir"
RANGE_BORROW_BODY="$ARTIFACT_DIR/range_loop_borrow_array.mir"
RANGE_REPLACE_BODY="$ARTIFACT_DIR/range_loop_replace_array_cell.mir"
TAIL_RETURN_BODY="$ARTIFACT_DIR/tail_return_array.mir"
TAIL_RETURN_ENTRY_BODY="$ARTIFACT_DIR/tail_return_array_entry.mir"
LOCAL_PARTIAL_BODY="$ARTIFACT_DIR/local_partial.mir"
MAKE_ADDER_BODY="$ARTIFACT_DIR/make_adder.mir"
ESCAPED_PARTIAL_BODY="$ARTIFACT_DIR/escaped_partial.mir"
CLOSURE_CAPTURED_ARRAY_BODY="$ARTIFACT_DIR/closure_captured_array_cell.mir"
CLOSURE_USE_CELL_BODY="$ARTIFACT_DIR/use_cell.mir"
MAYBE_ARRAY_BODY="$ARTIFACT_DIR/maybe_array.mir"
VARIANT_RETURN_BODY="$ARTIFACT_DIR/variant_payload_return.mir"
VARIANT_DISCARD_BODY="$ARTIFACT_DIR/variant_payload_discard.mir"
VARIANT_CONSUME_BODY="$ARTIFACT_DIR/variant_payload_consume.mir"
MAYBE_ARRAY_LIST_BODY="$ARTIFACT_DIR/maybe_array_list.mir"
VARIANT_LIST_HEAD_BODY="$ARTIFACT_DIR/variant_list_payload_consume_head.mir"
TUPLE_LOCAL_BODY="$ARTIFACT_DIR/tuple_local_dead.mir"
TUPLE_RETURN_BODY="$ARTIFACT_DIR/tuple_return_contains_array.mir"
TUPLE_PASSED_BODY="$ARTIFACT_DIR/tuple_passed_to_owner.mir"
TUPLE_STORED_BODY="$ARTIFACT_DIR/tuple_stored_in_aggregate.mir"
VARIANT_LOCAL_BODY="$ARTIFACT_DIR/variant_local_dead.mir"
VARIANT_RETURNED_BODY="$ARTIFACT_DIR/variant_returned.mir"
VARIANT_PASSED_BODY="$ARTIFACT_DIR/variant_passed_to_owner.mir"
VARIANT_STORED_BODY="$ARTIFACT_DIR/variant_stored_in_aggregate.mir"
LIST_LOCAL_BODY="$ARTIFACT_DIR/list_local_dead.mir"
LIST_RETURNED_BODY="$ARTIFACT_DIR/list_returned.mir"
LIST_PASSED_BODY="$ARTIFACT_DIR/list_passed_to_owner.mir"
LIST_STORED_BODY="$ARTIFACT_DIR/list_stored_in_aggregate.mir"
NESTED_EXTRACT_BODY="$ARTIFACT_DIR/record_nested_array_extract.mir"
TUPLE_DESTRUCTURE_BODY="$ARTIFACT_DIR/tuple_destructure_extract.mir"
LIST_HEAD_BODY="$ARTIFACT_DIR/list_head_extract.mir"
ARRAY_FILL_CONST_BODY="$ARTIFACT_DIR/array_fill_const_ints.mir"
ARRAY_FILL_BODY="$ARTIFACT_DIR/array_fill_ints.mir"
ARRAY_RANGE_BODY="$ARTIFACT_DIR/array_range_borrow.mir"
ARRAY_OFFSET_BODY="$ARTIFACT_DIR/array_offset_borrow.mir"
ARRAY_SUCC_BODY="$ARTIFACT_DIR/array_succ_borrow.mir"
TUPLE_OPTION_PAYLOAD_BODY="$ARTIFACT_DIR/tuple_option_payload_match.mir"
TUPLE_OPTION_LITERAL_BODY="$ARTIFACT_DIR/tuple_option_literal_match.mir"
RECORD_LIST_HEAD_BODY="$ARTIFACT_DIR/record_list_head_match.mir"
RECORD_LIST_EXACT_BODY="$ARTIFACT_DIR/record_list_exact_pair_match.mir"
RECORD_ARRAY_BODY="$ARTIFACT_DIR/record_array_match.mir"
NESTED_RECORD_LIST_BODY="$ARTIFACT_DIR/nested_record_list_match.mir"
NESTED_RECORD_ARRAY_BODY="$ARTIFACT_DIR/nested_record_array_match.mir"
TUPLE_LET_RECORD_BODY="$ARTIFACT_DIR/tuple_let_record_destructure.mir"
TUPLE_LET_PAIR_BODY="$ARTIFACT_DIR/tuple_let_pair_destructure.mir"
TUPLE_LET_NESTED_BODY="$ARTIFACT_DIR/tuple_let_nested_record_destructure.mir"
TUPLE_LET_MANAGED_BODY="$ARTIFACT_DIR/tuple_let_managed_record_payload.mir"
LIST_RECORD_ARRAY_PAYLOAD_BODY="$ARTIFACT_DIR/list_record_array_payload_match.mir"
ARRAY_RECORD_LIST_PAYLOAD_BODY="$ARTIFACT_DIR/array_record_list_payload_match.mir"
VARIANT_RECORD_PAYLOAD_BODY="$ARTIFACT_DIR/variant_record_payload_match.mir"
VARIANT_LITERAL_FAILURE_BODY="$ARTIFACT_DIR/variant_tuple_literal_failure_match.mir"
VARIANT_FIRST_FIELD_FAILURE_BODY="$ARTIFACT_DIR/variant_first_field_failure_payload_match.mir"
ARRAY_PATTERN_FAILURE_BODY="$ARTIFACT_DIR/array_pattern_failure_then_payload_match.mir"
LIST_PATTERN_FAILURE_BODY="$ARTIFACT_DIR/list_pattern_failure_then_payload_match.mir"
HEAP_ARRAY_PATTERN_FAILURE_BODY="$ARTIFACT_DIR/heap_array_pattern_failure_then_payload_match.mir"
HEAP_LIST_PATTERN_FAILURE_BODY="$ARTIFACT_DIR/heap_list_pattern_failure_then_payload_match.mir"
DIRECT_LEAF_BODY="$ARTIFACT_DIR/direct_leaf.mir"
DIRECT_PAIR_BODY="$ARTIFACT_DIR/direct_pair.mir"
DIRECT_ARRAY_LEAF_BODY="$ARTIFACT_DIR/direct_array_leaf.mir"
DIRECT_ARRAY_PAIR_BODY="$ARTIFACT_DIR/direct_array_pair.mir"
DIRECT_RECURSIVE_LIST_BODY="$ARTIFACT_DIR/direct_recursive_record_list_match.mir"
DIRECT_RECURSIVE_ARRAY_BODY="$ARTIFACT_DIR/direct_recursive_record_array_match.mir"
TENSOR_ADD_BODY="$ARTIFACT_DIR/tensor_add.mir"
TENSOR_OP_BODY="$ARTIFACT_DIR/tensor_op.mir"
TENSOR_ADD_CONSTRUCTOR_BODY="$ARTIFACT_DIR/tensor_add_constructor.mir"
TENSOR_OP_ADD_MATCH_BODY="$ARTIFACT_DIR/tensor_op_add_payload_match.mir"
TENSOR_OP_ADD_CONSUME_BODY="$ARTIFACT_DIR/tensor_op_add_payload_consume.mir"
COROUTINE_MAP_LOOP_BODY="$ARTIFACT_DIR/coroutine_map_loop.mir"
COROUTINE_ZIP_LOOP_BODY="$ARTIFACT_DIR/coroutine_zip_loop.mir"
COROUTINE_RETURNED_BODY="$ARTIFACT_DIR/coroutine_returned.mir"
COROUTINE_MAP_RETURNED_BODY="$ARTIFACT_DIR/coroutine_map_returned.mir"
COROUTINE_DUP_RETURNED_BODY="$ARTIFACT_DIR/coroutine_duplicate_returned.mir"

extract_function "$OWN_MIR" "duplicated_array" "$DUP_BODY"
extract_function "$OWN_MIR" "moved_call_arg" "$MOVED_BODY"
extract_function "$OWN_MIR" "returned_array" "$RETURNED_BODY"
extract_function "$OWN_MIR" "borrowed_array_size" "$BORROWED_BODY"
extract_function "$OWN_MIR" "branch_drop_returned_array" "$BRANCH_BODY"
extract_function "$OWN_MIR" "branch_consume_array" "$BRANCH_CONSUME_BODY"
extract_function "$OWN_MIR" "branch_return_array" "$BRANCH_RETURN_BODY"
extract_function "$OWN_MIR" "phi_select_preexisting_array_consume" "$PHI_CONSUME_BODY"
extract_function "$OWN_MIR" "phi_select_preexisting_array_duplicate" "$PHI_DUP_BODY"
extract_function "$OWN_MIR" "phi_select_preexisting_array_borrow_return" "$PHI_BORROW_RETURN_BODY"
extract_function "$OWN_MIR" "match_unused_heap_result" "$MATCH_BODY"
extract_function "$OWN_MIR" "projected_list" "$EXTRACTED_BODY"
extract_function "$OWN_MIR" "range_loop_borrow_array" "$RANGE_BORROW_BODY"
extract_function "$OWN_MIR" "range_loop_replace_array_cell" "$RANGE_REPLACE_BODY"
extract_function "$OWN_MIR" "tail_return_array" "$TAIL_RETURN_BODY"
extract_function "$OWN_MIR" "tail_return_array_entry" "$TAIL_RETURN_ENTRY_BODY"
extract_function "$CLOSURE_MIR" "local_partial" "$LOCAL_PARTIAL_BODY"
extract_function "$CLOSURE_MIR" "make_adder" "$MAKE_ADDER_BODY"
extract_function "$CLOSURE_MIR" "escaped_partial" "$ESCAPED_PARTIAL_BODY"
extract_function "$CLOSURE_MIR" "closure_captured_array_cell" "$CLOSURE_CAPTURED_ARRAY_BODY"
extract_function "$CLOSURE_MIR" "closure_captured_array_cell.use_cell" "$CLOSURE_USE_CELL_BODY"
extract_function "$VARIANT_MIR" "maybe_array" "$MAYBE_ARRAY_BODY"
extract_function "$VARIANT_MIR" "variant_payload_return" "$VARIANT_RETURN_BODY"
extract_function "$VARIANT_MIR" "variant_payload_discard" "$VARIANT_DISCARD_BODY"
extract_function "$VARIANT_MIR" "variant_payload_consume" "$VARIANT_CONSUME_BODY"
extract_function "$VARIANT_MIR" "maybe_array_list" "$MAYBE_ARRAY_LIST_BODY"
extract_function "$VARIANT_MIR" "variant_list_payload_consume_head" "$VARIANT_LIST_HEAD_BODY"
extract_function "$CONSTRUCT_MIR" "tuple_local_dead" "$TUPLE_LOCAL_BODY"
extract_function "$CONSTRUCT_MIR" "tuple_return_contains_array" "$TUPLE_RETURN_BODY"
extract_function "$CONSTRUCT_MIR" "tuple_passed_to_owner" "$TUPLE_PASSED_BODY"
extract_function "$CONSTRUCT_MIR" "tuple_stored_in_aggregate" "$TUPLE_STORED_BODY"
extract_function "$CONSTRUCT_MIR" "variant_local_dead" "$VARIANT_LOCAL_BODY"
extract_function "$CONSTRUCT_MIR" "variant_returned" "$VARIANT_RETURNED_BODY"
extract_function "$CONSTRUCT_MIR" "variant_passed_to_owner" "$VARIANT_PASSED_BODY"
extract_function "$CONSTRUCT_MIR" "variant_stored_in_aggregate" "$VARIANT_STORED_BODY"
extract_function "$CONSTRUCT_MIR" "list_local_dead" "$LIST_LOCAL_BODY"
extract_function "$CONSTRUCT_MIR" "list_returned" "$LIST_RETURNED_BODY"
extract_function "$CONSTRUCT_MIR" "list_passed_to_owner" "$LIST_PASSED_BODY"
extract_function "$CONSTRUCT_MIR" "list_stored_in_aggregate" "$LIST_STORED_BODY"
extract_function "$CONSTRUCT_MIR" "record_nested_array_extract" "$NESTED_EXTRACT_BODY"
extract_function "$CONSTRUCT_MIR" "tuple_destructure_extract" "$TUPLE_DESTRUCTURE_BODY"
extract_function "$CONSTRUCT_MIR" "list_head_extract" "$LIST_HEAD_BODY"
extract_function "$ARRAY_OP_MIR" "array_fill_const_ints" "$ARRAY_FILL_CONST_BODY"
extract_function "$ARRAY_OP_MIR" "array_fill_ints" "$ARRAY_FILL_BODY"
extract_function "$ARRAY_OP_MIR" "array_range_borrow" "$ARRAY_RANGE_BODY"
extract_function "$ARRAY_OP_MIR" "array_offset_borrow" "$ARRAY_OFFSET_BODY"
extract_function "$ARRAY_OP_MIR" "array_succ_borrow" "$ARRAY_SUCC_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "tuple_option_payload_match" "$TUPLE_OPTION_PAYLOAD_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "tuple_option_literal_match" "$TUPLE_OPTION_LITERAL_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "record_list_head_match" "$RECORD_LIST_HEAD_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "record_list_exact_pair_match" "$RECORD_LIST_EXACT_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "record_array_match" "$RECORD_ARRAY_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "nested_record_list_match" "$NESTED_RECORD_LIST_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "nested_record_array_match" "$NESTED_RECORD_ARRAY_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "tuple_let_record_destructure" "$TUPLE_LET_RECORD_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "tuple_let_pair_destructure" "$TUPLE_LET_PAIR_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "tuple_let_nested_record_destructure" "$TUPLE_LET_NESTED_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "tuple_let_managed_record_payload" "$TUPLE_LET_MANAGED_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "list_record_array_payload_match" "$LIST_RECORD_ARRAY_PAYLOAD_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "array_record_list_payload_match" "$ARRAY_RECORD_LIST_PAYLOAD_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "variant_record_payload_match" "$VARIANT_RECORD_PAYLOAD_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "variant_tuple_literal_failure_match" "$VARIANT_LITERAL_FAILURE_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "variant_first_field_failure_payload_match" "$VARIANT_FIRST_FIELD_FAILURE_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "array_pattern_failure_then_payload_match" "$ARRAY_PATTERN_FAILURE_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "list_pattern_failure_then_payload_match" "$LIST_PATTERN_FAILURE_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "heap_array_pattern_failure_then_payload_match" "$HEAP_ARRAY_PATTERN_FAILURE_BODY"
extract_function "$MATCH_DESTRUCTURE_MIR" "heap_list_pattern_failure_then_payload_match" "$HEAP_LIST_PATTERN_FAILURE_BODY"
extract_function "$RECURSIVE_DESTRUCTURE_MIR" "direct_leaf" "$DIRECT_LEAF_BODY"
extract_function "$RECURSIVE_DESTRUCTURE_MIR" "direct_pair" "$DIRECT_PAIR_BODY"
extract_function "$RECURSIVE_DESTRUCTURE_MIR" "direct_array_leaf" "$DIRECT_ARRAY_LEAF_BODY"
extract_function "$RECURSIVE_DESTRUCTURE_MIR" "direct_array_pair" "$DIRECT_ARRAY_PAIR_BODY"
extract_function "$RECURSIVE_DESTRUCTURE_MIR" "direct_recursive_record_list_match" "$DIRECT_RECURSIVE_LIST_BODY"
extract_function "$RECURSIVE_DESTRUCTURE_MIR" "direct_recursive_record_array_match" "$DIRECT_RECURSIVE_ARRAY_BODY"
extract_function "$RECURSIVE_DESTRUCTURE_MIR" "tensor_add" "$TENSOR_ADD_BODY"
extract_function "$RECURSIVE_DESTRUCTURE_MIR" "tensor_op" "$TENSOR_OP_BODY"
extract_function "$RECURSIVE_DESTRUCTURE_MIR" "tensor_add_constructor" "$TENSOR_ADD_CONSTRUCTOR_BODY"
extract_function "$RECURSIVE_DESTRUCTURE_MIR" "tensor_op_add_payload_match" "$TENSOR_OP_ADD_MATCH_BODY"
extract_function "$RECURSIVE_DESTRUCTURE_MIR" "tensor_op_add_payload_consume" "$TENSOR_OP_ADD_CONSUME_BODY"
extract_function "$COROUTINE_MIR" "coroutine_map_loop" "$COROUTINE_MAP_LOOP_BODY"
extract_function "$COROUTINE_MIR" "coroutine_zip_loop" "$COROUTINE_ZIP_LOOP_BODY"
extract_function "$COROUTINE_MIR" "coroutine_returned" "$COROUTINE_RETURNED_BODY"
extract_function "$COROUTINE_MIR" "coroutine_map_returned" "$COROUTINE_MAP_RETURNED_BODY"
extract_function "$COROUTINE_MIR" "coroutine_duplicate_returned" "$COROUTINE_DUP_RETURNED_BODY"

ARRAY_SIZE_MIR='extract\.field %[0-9]+, 0 : Int ; ops \[%[0-9]+:container/borrow#0\]'
ARRAY_DATA_MIR='extract\.field %[0-9]+, 2 : Ptr ; ops \[%[0-9]+:container/borrow#0\]'
ARRAY_PTR_OFFSET_MIR='ptr_offset %[0-9]+, %[0-9]+ : Ptr ; ops \[%[0-9]+:value/borrow#0, %[0-9]+:index/borrow#1\]'
ARRAY_LOAD_MIR='load %[0-9]+ :'
ARRAY_VIEW_MIR='construct\.tuple \{ %[0-9]+, %[0-9]+, %[0-9]+ \} : Array.*ops \[%[0-9]+:field/borrow#0, %[0-9]+:field/borrow#1, %[0-9]+:field/borrow#2\]'

section "MIR generation"
assert_contains "$RANGE_BORROW_BODY" '^  bb1 loop\.cond:' "range loop should create a loop condition block"
assert_contains "$RANGE_BORROW_BODY" '^    %[0-9]+ = phi \[bb0: %[0-9]+, bb3: %[0-9]+\]' "range loop should lower the induction variable as MIR_PHI"
assert_contains "$RANGE_BORROW_BODY" '^  bb2 loop\.body:' "range loop should create a loop body block"
assert_contains "$RANGE_BORROW_BODY" '^  bb3 loop\.inc:' "range loop should create a loop increment block"
assert_contains "$RANGE_BORROW_BODY" '^  bb4 loop\.after:' "range loop should create a loop exit block"
assert_contains "$RANGE_BORROW_BODY" "$ARRAY_SIZE_MIR" "array size projection should borrow its container"
assert_contains "$BRANCH_BODY" '^  bb1 match\.cont:' "if expression should lower through a MIR match continuation"
assert_contains "$BRANCH_BODY" '^    %[0-9]+ = phi \[bb2: %[0-9]+, bb3: %[0-9]+\]' "match result should be a MIR_PHI in the continuation"
assert_contains "$BRANCH_BODY" '^    br bb1$' "match arms should branch into the continuation"
assert_contains "$BRANCH_RETURN_BODY" '^    %[0-9]+ = phi \[bb2: %[0-9]+, bb3: %[0-9]+\].*Array' "branch-produced managed values should join through MIR_PHI"
assert_contains "$PHI_CONSUME_BODY" '^    %[0-9]+ = phi \[bb2: %[0-9]+, bb3: %[0-9]+\].*Array' "preexisting managed values should join through a non-induction MIR_PHI"
assert_contains "$PHI_DUP_BODY" '^    %[0-9]+ = phi \[bb2: %[0-9]+, bb3: %[0-9]+\].*Array' "duplicated managed phi fixture should lower through MIR_PHI"
assert_contains "$PHI_BORROW_RETURN_BODY" '^    %[0-9]+ = phi \[bb2: %[0-9]+, bb3: %[0-9]+\].*Array' "borrowed-return managed phi fixture should lower through MIR_PHI"
assert_contains "$TAIL_RETURN_BODY" '^    %[0-9]+ = phi \[bb2: %[0-9]+, bb3: %[0-9]+\]' "tail-recursive managed transfer should join through MIR_PHI"
assert_contains "$DUP_BODY" 'construct\.tuple \{ %[0-9]+, %[0-9]+ \}.*ops \[%[0-9]+:field/consume#0, %[0-9]+:field/consume#1\]' "tuple construction should consume stored managed fields"
assert_contains "$DUP_BODY" 'return %[0-9]+.*ops \[%[0-9]+:return/consume#0\]' "MIR return should consume owned return values"
assert_contains "$LOCAL_PARTIAL_BODY" 'construct\.closure_env \{ %[0-9]+ \}.*ops \[%[0-9]+:field/consume#0\]' "partial application should lower captured values into closure_env"
assert_contains "$LOCAL_PARTIAL_BODY" 'extract\.closure_env %[0-9]+.*ops \[%[0-9]+:value/borrow#0\]' "closure environment extraction should borrow the closure"
assert_contains "$LOCAL_PARTIAL_BODY" 'extract\.closure_fn %[0-9]+.*ops \[%[0-9]+:value/borrow#0\]' "closure function extraction should borrow the closure"
assert_contains "$CLOSURE_MIR" 'extract\.field %[0-9]+, 0.*ops \[%[0-9]+:container/borrow#0\]' "closure body should borrow from its extracted environment field"
assert_order "$MAYBE_ARRAY_BODY" "variant construction should consume managed payload fields" \
  'construct\.variant Some#0\(%[0-9]+\)' \
  'ops \[%[0-9]+:field/consume#0\]'
assert_contains "$MAYBE_ARRAY_BODY" 'construct\.variant None#1\(\)' "nullary variant construction should use MIR_CONSTRUCT"
assert_contains "$VARIANT_RETURN_BODY" 'extract\.variant_tag %[0-9]+.*ops \[%[0-9]+:value/borrow#0\]' "variant tag extraction should borrow the scrutinee"
assert_contains "$VARIANT_RETURN_BODY" 'extract\.variant_payload %[0-9]+, Some#0.*ops \[%[0-9]+:container/borrow#0\]' "variant payload extraction should borrow the scrutinee"
assert_contains "$VARIANT_RETURN_BODY" 'extract\.field %[0-9]+, 0.*ops \[%[0-9]+:container/borrow#0\]' "variant payload field extraction should borrow the payload"
assert_contains "$VARIANT_RETURN_BODY" '^    %[0-9]+ = phi \[bb4: %[0-9]+, bb7: %[0-9]+\]' "managed variant match result should lower through MIR_PHI"
assert_order "$VARIANT_LIST_HEAD_BODY" "variant payload containing a list should extract payload, field, then list head" \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.list_head %[0-9]+'
assert_order "$TUPLE_OPTION_PAYLOAD_BODY" "option tuple payload match should extract variant payload and both tuple fields" \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+' \
  'extract\.field %[0-9]+, 1'
assert_contains "$TUPLE_OPTION_PAYLOAD_BODY" 'call %[0-9]+\(%[0-9]+\).*: Int.*value/consume#0' "option tuple payload arm should consume the selected managed field"
assert_order "$TUPLE_OPTION_LITERAL_BODY" "option tuple literal match should compare both tuple fields" \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'const\.int 1' \
  'ieq %[0-9]+, %[0-9]+' \
  'extract\.field %[0-9]+, 1' \
  'const\.double 1' \
  'feq %[0-9]+, %[0-9]+'
assert_order "$RECORD_LIST_HEAD_BODY" "record field projection from list head destructure should lower through extract.field" \
  '^  bb4 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  'br bb2'
assert_order "$RECORD_LIST_HEAD_BODY" "list head record destructure should extract head and tail before entering the body" \
  'extract\.list_head %[0-9]+' \
  'extract\.list_tail %[0-9]+' \
  '^  bb7 match\.list_tail\.bb3:' \
  'br bb4'
assert_order "$RECORD_LIST_EXACT_BODY" "exact two-cons record destructure should project both bound records" \
  '^  bb4 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'iadd %[0-9]+, %[0-9]+'
assert_order "$RECORD_LIST_EXACT_BODY" "exact two-cons record destructure should test the second tail before entering the body" \
  'extract\.list_head %[0-9]+' \
  'extract\.list_tail %[0-9]+' \
  'list_is_empty %[0-9]+' \
  'extract\.list_head %[0-9]+' \
  'extract\.list_tail %[0-9]+' \
  'list_is_empty %[0-9]+' \
  'cond %[0-9]+, bb4, bb5'
assert_order "$RECORD_ARRAY_BODY" "record array pattern should test length and extract both elements" \
  "$ARRAY_SIZE_MIR" \
  'const\.int 2' \
  'ieq %[0-9]+, %[0-9]+' \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR" \
  'br bb7' \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR" \
  'br bb4'
assert_order "$RECORD_ARRAY_BODY" "record array pattern should project both bound record fields" \
  '^  bb4 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'iadd %[0-9]+, %[0-9]+'
assert_not_contains "$RECORD_ARRAY_BODY" '<no terminator>' "record array pattern should not leave unterminated MIR blocks"
assert_order "$NESTED_RECORD_LIST_BODY" "nested record list destructure should project through both record layers" \
  '^  bb4 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'iadd %[0-9]+, %[0-9]+'
assert_order "$NESTED_RECORD_LIST_BODY" "nested record list destructure should test both cons cells before the body" \
  'extract\.list_head %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'extract\.list_tail %[0-9]+' \
  'ops \[%[0-9]+:container/borrow#0\]' \
  'list_is_empty %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'extract\.list_head %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'extract\.list_tail %[0-9]+' \
  'ops \[%[0-9]+:container/borrow#0\]' \
  'list_is_empty %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]'
assert_order "$NESTED_RECORD_ARRAY_BODY" "nested record array destructure should test length and extract both elements" \
  "$ARRAY_SIZE_MIR" \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR" \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR"
assert_order "$NESTED_RECORD_ARRAY_BODY" "nested record array destructure should project through both record layers" \
  '^  bb4 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0'
assert_order "$TUPLE_LET_RECORD_BODY" "tuple let-binding destructure should bind fields through MIR_EXTRACT" \
  'construct\.tuple \{ %[0-9]+, %[0-9]+ \}' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 1' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'iadd %[0-9]+, %[0-9]+'
assert_order "$TUPLE_LET_PAIR_BODY" "plain tuple let-binding destructure should extract both tuple fields" \
  'construct\.tuple \{ %[0-9]+, %[0-9]+ \}' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 1' \
  'iadd %[0-9]+, %[0-9]+'
assert_order "$TUPLE_LET_NESTED_BODY" "nested record tuple let-binding destructure should extract through both record layers" \
  'construct\.tuple \{ node: %[0-9]+ \}' \
  'construct\.tuple \{ node: %[0-9]+ \}' \
  'construct\.tuple \{ %[0-9]+, %[0-9]+ \}' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 1' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'iadd %[0-9]+, %[0-9]+'
assert_order "$TUPLE_LET_MANAGED_BODY" "managed tuple let-binding destructure should extract payload before consuming it" \
  'construct\.tuple \{ payload: %[0-9]+ \}' \
  'construct\.tuple \{ payload: %[0-9]+ \}' \
  'construct\.tuple \{ %[0-9]+, %[0-9]+ \}' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 1' \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+' \
  'call %[0-9]+.*value/consume#0'
assert_order "$LIST_RECORD_ARRAY_PAYLOAD_BODY" "list pattern should bind a record containing an array payload" \
  'construct\.list_cons %[0-9]+, %[0-9]+' \
  'extract\.list_head %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'extract\.list_tail %[0-9]+' \
  'ops \[%[0-9]+:container/borrow#0\]'
assert_order "$LIST_RECORD_ARRAY_PAYLOAD_BODY" "list-bound record array payload should be consumed in the arm body" \
  '^  bb4 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  'call %[0-9]+.*consume_array.*value/consume#0'
assert_order "$ARRAY_RECORD_LIST_PAYLOAD_BODY" "array pattern should bind a record containing a list payload" \
  "$ARRAY_SIZE_MIR" \
  'const\.int 1' \
  'ieq %[0-9]+, %[0-9]+' \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR"
assert_order "$ARRAY_RECORD_LIST_PAYLOAD_BODY" "array-bound record list payload should be consumed in the arm body" \
  '^  bb4 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  'call %[0-9]+.*consume_list.*value/consume#0'
assert_order "$VARIANT_RECORD_PAYLOAD_BODY" "variant record payload match should extract the record payload before entering the arm" \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'extract\.field %[0-9]+, 0' \
  'br bb[0-9]+'
assert_order "$VARIANT_RECORD_PAYLOAD_BODY" "variant record payload arm should consume the extracted managed field" \
  '^  bb7 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+' \
  'call %[0-9]+.*value/consume#0'
assert_order "$VARIANT_LITERAL_FAILURE_BODY" "failed literal branch should drop the duplicated payload before retrying the next arm" \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+' \
  'const\.int 2' \
  'ieq %[0-9]+, %[0-9]+' \
  'perceus\.edge\.drop:' \
  ' = drop %[0-9]+' \
  'br bb[0-9]+'
assert_order "$VARIANT_FIRST_FIELD_FAILURE_BODY" "variant first-field literal failure should not own the managed payload before the failed compare" \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'const\.int 2' \
  'ieq %[0-9]+, %[0-9]+' \
  'cond %[0-9]+, bb10, bb8' \
  '^  bb10 match\.tuple\.[0-9]+\.1:' \
  'extract\.field %[0-9]+, 1' \
  ' = dup %[0-9]+'
assert_not_contains "$VARIANT_FIRST_FIELD_FAILURE_BODY" 'perceus\.edge\.drop' "variant first-field failure should not emit an edge drop before payload ownership exists"
assert_order "$ARRAY_PATTERN_FAILURE_BODY" "failed array length pattern should retry before extracting the one-element payload" \
  "$ARRAY_SIZE_MIR" \
  'const\.int 2' \
  'ieq %[0-9]+, %[0-9]+' \
  '^  bb5 match\.arm\.1\.test:' \
  "$ARRAY_SIZE_MIR" \
  'const\.int 1' \
  'ieq %[0-9]+, %[0-9]+' \
  '^  bb10 match\.array\.bb5:' \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR"
assert_contains "$ARRAY_PATTERN_FAILURE_BODY" 'call %[0-9]+.*value/consume#0' "array pattern fallback arm should consume the extracted managed payload"
assert_order "$LIST_PATTERN_FAILURE_BODY" "failed two-cons list pattern should retry against the one-cons payload arm" \
  'extract\.list_head %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'extract\.list_tail %[0-9]+' \
  'ops \[%[0-9]+:container/borrow#0\]' \
  'list_is_empty %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'extract\.list_head %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'extract\.list_tail %[0-9]+' \
  'ops \[%[0-9]+:container/borrow#0\]' \
  'list_is_empty %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'cond %[0-9]+, bb4, bb5' \
  '^  bb10 match\.arm\.1\.body:' \
  'extract\.field %[0-9]+, 0' \
  'call %[0-9]+.*consume_array.*value/consume#0'
assert_order "$LIST_PATTERN_FAILURE_BODY" "one-cons fallback list pattern should test its tail before consuming payload" \
  '^  bb12 match\.list_cons\.bb5:' \
  'extract\.list_head %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'extract\.list_tail %[0-9]+' \
  'ops \[%[0-9]+:container/borrow#0\]' \
  '^  bb13 match\.list_tail\.bb5:' \
  'list_is_empty %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'cond %[0-9]+, bb10, bb11'
assert_order "$HEAP_ARRAY_PATTERN_FAILURE_BODY" "heap array pattern failure should drop the owned scrutinee on the no-match retry edge" \
  "$ARRAY_SIZE_MIR" \
  'const\.int 2' \
  'ieq %[0-9]+, %[0-9]+' \
  '^  bb5 match\.arm\.1\.test:' \
  "$ARRAY_SIZE_MIR" \
  'const\.int 1' \
  'ieq %[0-9]+, %[0-9]+' \
  'perceus\.edge\.drop:' \
  ' = drop %[0-9]+'
assert_order "$HEAP_LIST_PATTERN_FAILURE_BODY" "heap list pattern failure should drop duplicated borrowed tails on retry edges" \
  'extract\.list_tail %[0-9]+' \
  'ops \[%[0-9]+:container/borrow#0\]' \
  ' = dup %[0-9]+' \
  'list_is_empty %[0-9]+' \
  'perceus\.edge\.drop:' \
  ' = drop %[0-9]+' \
  'br bb5'
assert_order "$DIRECT_LEAF_BODY" "recursive record list leaf constructor should lower as list_empty plus construct.tuple" \
  'construct\.list_empty' \
  'construct\.tuple \{ value: %[0-9]+, kids: %[0-9]+ \}'
assert_order "$DIRECT_PAIR_BODY" "recursive record list pair constructor should lower list cons cells into the record field" \
  'construct\.list_empty' \
  'construct\.list_cons %[0-9]+, %[0-9]+' \
  'construct\.list_cons %[0-9]+, %[0-9]+' \
  'construct\.tuple \{ value: %[0-9]+, kids: %[0-9]+ \}'
assert_order "$DIRECT_ARRAY_LEAF_BODY" "recursive record array leaf constructor should lower empty array into the record field" \
  'construct\.array_literal \{  \}' \
  'construct\.tuple \{ value: %[0-9]+, kids: %[0-9]+ \}'
assert_order "$DIRECT_ARRAY_PAIR_BODY" "recursive record array pair constructor should lower array literal into the record field" \
  'construct\.array_literal \{ %[0-9]+, %[0-9]+ \}' \
  'construct\.tuple \{ value: %[0-9]+, kids: %[0-9]+ \}'
assert_order "$DIRECT_RECURSIVE_LIST_BODY" "recursive list record match should extract kids before testing the list" \
  'extract\.field %[0-9]+, 1' \
  'list_is_empty %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'extract\.list_head %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' \
  'extract\.list_tail %[0-9]+'
assert_order "$DIRECT_RECURSIVE_LIST_BODY" "recursive list record match should project both matched node values" \
  '^  bb4 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'iadd %[0-9]+, %[0-9]+'
assert_order "$DIRECT_RECURSIVE_ARRAY_BODY" "recursive array record match should extract kids before testing the array" \
  'extract\.field %[0-9]+, 1' \
  "$ARRAY_SIZE_MIR" \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR" \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR"
assert_order "$DIRECT_RECURSIVE_ARRAY_BODY" "recursive array record match should project both matched node values" \
  '^  bb4 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'iadd %[0-9]+, %[0-9]+'
assert_not_contains "$DIRECT_RECURSIVE_LIST_BODY" '<no terminator>' "recursive list record match should not leave unterminated MIR blocks"
assert_not_contains "$DIRECT_RECURSIVE_ARRAY_BODY" '<no terminator>' "recursive array record match should not leave unterminated MIR blocks"
assert_order "$TENSOR_ADD_BODY" "recursive TensorRef sum constructor should consume tuple payload fields" \
  'construct\.tuple \{ %[0-9]+, %[0-9]+ \}' \
  'ops \[%[0-9]+:field/consume#0, %[0-9]+:field/consume#1\]' \
  'construct\.variant TensOpAdd#1\(%[0-9]+\)' \
  'ops \[%[0-9]+:field/consume#0\]' \
  'construct\.tuple \{ data: %[0-9]+, grad: %[0-9]+, shape: %[0-9]+, op: %[0-9]+ \}' \
  'ops \[%[0-9]+:field/consume#0, %[0-9]+:field/consume#1, %[0-9]+:field/consume#2, %[0-9]+:field/consume#3\]'
assert_order "$TENSOR_ADD_CONSTRUCTOR_BODY" "TensorRef constructor fixture should build two leaves before tensor_add" \
  'fn_ref \$tensor_make' \
  'call %[0-9]+( as \$tensor_make[^ ]*)?\(' \
  'fn_ref \$tensor_make' \
  'call %[0-9]+( as \$tensor_make[^ ]*)?\(' \
  'fn_ref \$tensor_add' \
  'call %[0-9]+( as \$tensor_add[^ ]*)?\(' \
  'return %[0-9]+'
assert_order "$TENSOR_OP_BODY" "TensorRef op projection should extract array element before op field" \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR" \
  'extract\.field %[0-9]+, 3' \
  'ops \[%[0-9]+:container/borrow#0\]'
assert_order "$TENSOR_OP_ADD_MATCH_BODY" "recursive sum payload match should extract both TensorRef fields" \
  'extract\.variant_payload %[0-9]+, TensOpAdd#1' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 1'
assert_order "$TENSOR_OP_ADD_MATCH_BODY" "recursive sum payload arm should borrow both TensorRef payloads for array_size" \
  '^  bb4 match\.arm\.0\.body:' \
  "$ARRAY_SIZE_MIR" \
  "$ARRAY_SIZE_MIR" \
  'iadd %[0-9]+, %[0-9]+'
assert_order "$TENSOR_OP_ADD_CONSUME_BODY" "recursive sum payload consuming arm should extract selected TensorRef field" \
  'extract\.variant_payload %[0-9]+, TensOpAdd#1' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 1'
assert_not_contains "$TENSOR_OP_ADD_MATCH_BODY" '<no terminator>' "recursive sum payload match should not leave unterminated MIR blocks"
assert_not_contains "$TENSOR_OP_ADD_CONSUME_BODY" '<no terminator>' "recursive sum payload consume should not leave unterminated MIR blocks"
assert_order "$NESTED_EXTRACT_BODY" "nested tuple/array extraction should lower as field extract before array_at" \
  'extract\.field %[0-9]+, 0' \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR"
assert_order "$CLOSURE_USE_CELL_BODY" "closure env array field should extract env field before array element" \
  'extract\.field %[0-9]+, 0' \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR"
assert_contains "$LIST_HEAD_BODY" 'extract\.list_head %[0-9]+.*ops \[%[0-9]+:container/borrow#0\]' "list head pattern should borrow the list container"
assert_order "$LIST_HEAD_BODY" "list tail pattern should borrow the list container" \
  'extract\.list_tail %[0-9]+' \
  'ops \[%[0-9]+:container/borrow#0\]'
assert_contains "$ARRAY_FILL_CONST_BODY" 'construct\.array_fill_const %[0-9]+, %[0-9]+.*ops \[%[0-9]+:value/borrow#0, %[0-9]+:element/consume#1\]' "array_fill_const should construct through MIR_CONSTRUCT with size borrow and element consume"
assert_contains "$ARRAY_FILL_BODY" 'construct\.array_fill %[0-9]+, %[0-9]+.*ops \[%[0-9]+:value/borrow#0, %[0-9]+:function/borrow#1\]' "array_fill should borrow the fill function"
assert_order "$ARRAY_RANGE_BODY" "array_range should lower to a borrowed data pointer slice" \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_VIEW_MIR"
assert_order "$ARRAY_OFFSET_BODY" "array_offset should lower to size/data pointer arithmetic" \
  "$ARRAY_SIZE_MIR" \
  'igt %[0-9]+, %[0-9]+' \
  'isub %[0-9]+, %[0-9]+' \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_VIEW_MIR"
assert_order "$ARRAY_SUCC_BODY" "array_succ should lower to size/data pointer arithmetic" \
  "$ARRAY_SIZE_MIR" \
  'igt %[0-9]+, %[0-9]+' \
  'isub %[0-9]+, %[0-9]+' \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_VIEW_MIR"
assert_order "$COROUTINE_MIR" "cor_map pipeline should construct iter, map, then loop coroutines" \
  '^fn coroutine_map_loop\(' \
  'fn_ref \$\$builtin\.iter\.list\.[0-9]+' \
  'coro\.new %[0-9]+\(%[0-9]+\)' \
  'fn_ref \$\$builtin\.cor_map\.[0-9]+' \
  'coro\.new %[0-9]+\(%[0-9]+, %[0-9]+\)' \
  'fn_ref \$\$builtin\.cor_loop\.[0-9]+' \
  'coro\.new %[0-9]+\(%[0-9]+\)'
assert_order "$COROUTINE_MIR" "cor_map wrapper should drive source and yield mapped values" \
  '^fn \$builtin\.cor_map\.[0-9]+\(' \
  '^  bb1 cor_map\.check:' \
  'coro\.next %[0-9]+\(\)' \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'extract\.field %[0-9]+, 0' \
  'call %0\(%[0-9]+\)' \
  'yield %[0-9]+, bb3' \
  '^  bb4 cor_map\.done:' \
  'coro\.done'
assert_order "$COROUTINE_MIR" "cor_loop over mapped coroutine should reset through coro.reset" \
  'coro\.reset %[0-9]+' \
  'ops \[%[0-9]+:value/consume#0\]'
assert_order "$COROUTINE_MIR" "cor_map should specialize captured generic builtin functions" \
  '^fn coroutine_map_double_loop\(' \
  'fn_ref \$builtin_Double\.Int\.Double' \
  'fn_ref \$\$builtin\.cor_map\.[0-9]+' \
  'coro\.new %[0-9]+\(%[0-9]+, %[0-9]+\)'
assert_order "$COROUTINE_MIR" "cor_zip pipeline should construct two sources, zip, then loop" \
  '^fn coroutine_zip_loop\(' \
  'fn_ref \$coroutine_zip_loop\.a' \
  'fn_ref \$coroutine_zip_loop\.b' \
  'coro\.new %[0-9]+\(\)' \
  'coro\.new %[0-9]+\(\)' \
  'fn_ref \$\$builtin\.cor_zip\.[0-9]+' \
  'coro\.new %[0-9]+\(%[0-9]+, %[0-9]+\)' \
  'fn_ref \$\$builtin\.cor_loop\.[0-9]+' \
  'coro\.new %[0-9]+\(%[0-9]+\)'
assert_order "$COROUTINE_MIR" "cor_zip wrapper should stop on either input and yield tuples" \
  '^fn \$builtin\.cor_zip\.[0-9]+\(' \
  '^  bb1 cor_zip\.check_left:' \
  'coro\.next %0\(\)' \
  'cond %[0-9]+, bb2, bb[0-9]+' \
  '^  bb2 cor_zip\.check_right:' \
  'coro\.next %1\(\)' \
  'cond %[0-9]+, bb3, bb[0-9]+' \
  '^  bb3 cor_zip\.value:' \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'construct\.tuple \{ %[0-9]+, %[0-9]+ \}' \
  'yield %[0-9]+, bb4' \
  '^  bb5 cor_zip\.done:' \
  'coro\.done'
assert_contains "$EXTERN_MIR" '^extern fn abs\(%0 arg0: T @borrow\) -> T @owned;' "generic extern should dump as a bodyless MIR function"
assert_contains "$EXTERN_MIR" '^extern fn abs\.Int\.Int\(%0 arg0: Int @borrow\) -> Int @owned;' "specialized extern should dump as a bodyless MIR function"

section "MIR escape analysis"
assert_contains "$RETURNED_BODY" 'construct\.array_literal \{ %[0-9]+, %[0-9]+ \}.*ea heap#[0-9]+' "returned arrays should be marked heap allocated"
assert_contains "$MOVED_BODY" 'construct\.array_literal \{ %[0-9]+, %[0-9]+ \}.*ea stack#[0-9]+' "non-escaping call arguments should remain stack allocated"
assert_contains "$EXTRACTED_BODY" 'construct\.list_cons %[0-9]+, %[0-9]+' "extracted list nodes should be present in MIR"
assert_contains "$EXTRACTED_BODY" 'construct\.array_literal \{ %[0-9]+ \}' "extracted array container should be present in MIR"
assert_contains "$EXTRACTED_BODY" 'ea stack#[0-9]+' "extracted stack-owned aggregate allocations should stay stack allocated"
assert_contains "$RANGE_REPLACE_BODY" 'construct\.array_literal \{ %[0-9]+ \}.*ea heap#[0-9]+ mutable' "mutated arrays that escape should be heap mutable"
assert_contains "$LOCAL_PARTIAL_BODY" 'construct\.closure_env \{ %[0-9]+ \}.*ea stack#[0-9]+' "locally invoked partial closure env should stay stack allocated"
assert_contains "$MAKE_ADDER_BODY" 'construct\.closure_env \{ %[0-9]+ \}.*ea heap#[0-9]+' "returned partial closure env should be heap allocated"
assert_contains "$TUPLE_LOCAL_BODY" 'construct\.array_literal \{ %[0-9]+, %[0-9]+ \}.*ea stack#[0-9]+' "array stored only in a dead tuple should stay stack allocated"
assert_contains "$TUPLE_RETURN_BODY" 'construct\.array_literal \{ %[0-9]+, %[0-9]+ \}.*ea heap#[0-9]+' "array stored in a returned tuple should be heap allocated"
assert_contains "$TUPLE_PASSED_BODY" 'construct\.array_literal \{ %[0-9]+, %[0-9]+ \}.*ea stack#[0-9]+' "array stored in a tuple passed to an owning call should stay stack allocated"
assert_contains "$TUPLE_PASSED_BODY" 'construct\.tuple \{ %[0-9]+, %[0-9]+ \}.*ops \[%[0-9]+:field/consume#0, %[0-9]+:field/consume#1\]' "tuple passed to owning call should consume its fields"
assert_contains "$TUPLE_PASSED_BODY" 'call %[0-9]+\(%[0-9]+\).*value/consume#0' "tuple passed to owning call should be consumed by the call"
assert_contains "$TUPLE_STORED_BODY" 'construct\.array_literal \{ %[0-9]+ \}.*ea stack#[0-9]+' "tuple stored in a dead aggregate should keep aggregate allocation stack local"
assert_contains "$VARIANT_LOCAL_BODY" 'construct\.array_literal \{ %[0-9]+, %[0-9]+ \}.*ea stack#[0-9]+' "array stored in a dead variant should stay stack allocated"
assert_contains "$VARIANT_RETURNED_BODY" 'construct\.array_literal \{ %[0-9]+, %[0-9]+ \}.*ea heap#[0-9]+' "array stored in a returned variant should be heap allocated"
assert_contains "$VARIANT_PASSED_BODY" 'construct\.array_literal \{ %[0-9]+, %[0-9]+ \}.*ea stack#[0-9]+' "array stored in a variant passed to an owning call should stay stack allocated"
assert_contains "$VARIANT_PASSED_BODY" 'call %[0-9]+\(%[0-9]+\).*value/consume#0' "variant passed to owning call should be consumed by the call"
assert_contains "$VARIANT_STORED_BODY" 'construct\.array_literal \{ %[0-9]+, %[0-9]+ \}.*ea heap#[0-9]+' "array stored in a returned aggregate variant should be heap allocated"
assert_order "$LIST_LOCAL_BODY" "dead list cons should stay stack allocated" \
  'construct\.list_cons %[0-9]+, %[0-9]+' \
  'ea stack#[0-9]+'
assert_order "$LIST_RETURNED_BODY" "returned list cons should be heap allocated" \
  'construct\.list_cons %[0-9]+, %[0-9]+' \
  'ea heap#[0-9]+'
assert_order "$LIST_PASSED_BODY" "list cons passed to owning call should stay stack allocated" \
  'construct\.list_cons %[0-9]+, %[0-9]+' \
  'ea stack#[0-9]+'
assert_contains "$LIST_PASSED_BODY" 'call %[0-9]+\(%[0-9]+\).*value/consume#0' "list passed to owning call should be consumed by the call"
assert_order "$LIST_STORED_BODY" "list cons stored in returned aggregate should be heap allocated" \
  'construct\.list_cons %[0-9]+, %[0-9]+' \
  'ea heap#[0-9]+'
assert_contains "$NESTED_EXTRACT_BODY" 'construct\.array_literal \{ %[0-9]+ \}.*ea stack#[0-9]+' "nested extraction from stack wrapper should keep wrapper allocation stack local"
assert_contains "$ARRAY_FILL_CONST_BODY" 'construct\.array_fill_const %[0-9]+, %[0-9]+.*ea heap#[0-9]+' "returned array_fill_const result should be heap allocated"
assert_contains "$ARRAY_FILL_BODY" 'construct\.array_fill %[0-9]+, %[0-9]+.*ea heap#[0-9]+' "returned array_fill result should be heap allocated"
assert_order "$DIRECT_PAIR_BODY" "returned recursive list cons fields should be heap allocated" \
  'construct\.list_cons %[0-9]+, %[0-9]+' \
  'ea heap#[0-9]+' \
  'construct\.list_cons %[0-9]+, %[0-9]+' \
  'ea heap#[0-9]+'
assert_contains "$DIRECT_ARRAY_PAIR_BODY" 'construct\.array_literal \{ %[0-9]+, %[0-9]+ \}.*ea heap#[0-9]+' "returned recursive array field should be heap allocated"
assert_order "$TENSOR_ADD_BODY" "returned TensorRef constructor should heap allocate the recursive node array" \
  'construct\.tuple \{ data: %[0-9]+, grad: %[0-9]+, shape: %[0-9]+, op: %[0-9]+ \}' \
  'construct\.array_literal \{ %[0-9]+ \}' \
  'ea heap#[0-9]+'
assert_order "$COROUTINE_MAP_LOOP_BODY" "local mapped coroutine pipeline should stay stack allocated" \
  'coro\.new %[0-9]+\(%[0-9]+\)' \
  'ea stack#[0-9]+' \
  'coro\.new %[0-9]+\(%[0-9]+, %[0-9]+\)' \
  'ea stack#[0-9]+' \
  'coro\.new %[0-9]+\(%[0-9]+\)' \
  'ea stack#[0-9]+'
assert_order "$COROUTINE_ZIP_LOOP_BODY" "local zipped coroutine pipeline should stay stack allocated" \
  'coro\.new %[0-9]+\(\)' \
  'ea stack#[0-9]+' \
  'coro\.new %[0-9]+\(\)' \
  'ea stack#[0-9]+' \
  'coro\.new %[0-9]+\(%[0-9]+, %[0-9]+\)' \
  'ea stack#[0-9]+' \
  'coro\.new %[0-9]+\(%[0-9]+\)' \
  'ea stack#[0-9]+'
assert_order "$COROUTINE_RETURNED_BODY" "returned coroutine should be heap allocated" \
  'coro\.new %[0-9]+\(\)' \
  'ea heap#[0-9]+' \
  'return %[0-9]+.*return/consume#0'
assert_order "$COROUTINE_MAP_RETURNED_BODY" "returned coroutine adapter should heap allocate wrapper and captured source" \
  'coro\.new %[0-9]+\(\)' \
  'ea heap#[0-9]+' \
  'coro\.new %[0-9]+\(%[0-9]+, %[0-9]+\)' \
  'ea heap#[0-9]+' \
  'return %[0-9]+.*return/consume#0'

section "Perceus instrumentation"
assert_order "$DUP_BODY" "duplicate array should dup before consuming the same owner twice" \
  'construct\.array_literal' \
  ' = dup %[0-9]+' \
  'construct\.tuple \{ %[0-9]+, %[0-9]+ \}'
assert_order "$COROUTINE_DUP_RETURNED_BODY" "duplicated returned coroutine should dup before consuming the owner twice" \
  'coro\.new %[0-9]+\(\)' \
  'ea heap#[0-9]+' \
  ' = dup %[0-9]+' \
  'construct\.tuple \{ %[0-9]+, %[0-9]+ \}' \
  'field/consume#0.*field/consume#1'
assert_order "$COROUTINE_MIR" "cor_map wrapper should drop the owned source on the done edge" \
  '^fn \$builtin\.cor_map\.[0-9]+\(' \
  'cond %[0-9]+, bb2, bb[0-9]+' \
  '^  bb4 cor_map\.done:' \
  'coro\.done' \
  '^  bb[0-9]+ perceus\.edge\.drop:' \
  ' = drop %1' \
  'br bb4'
assert_order "$COROUTINE_MIR" "cor_zip wrapper should drop both owned sources on either done edge" \
  '^fn \$builtin\.cor_zip\.[0-9]+\(' \
  '^  bb5 cor_zip\.done:' \
  'coro\.done' \
  '^  bb[0-9]+ perceus\.edge\.drop:' \
  ' = drop %0' \
  ' = drop %1' \
  'br bb5' \
  '^  bb[0-9]+ perceus\.edge\.drop:' \
  ' = drop %0' \
  ' = drop %1' \
  'br bb5'
assert_order "$BORROWED_BODY" "borrowed array size should project the size field before tuple return" \
  'construct\.array_literal' \
  "$ARRAY_SIZE_MIR" \
  'construct\.tuple \{ %[0-9]+, %[0-9]+ \}'
assert_contains "$BRANCH_BODY" '^  bb[0-9]+ perceus\.edge\.drop:' "path-local branch release should split the edge"
assert_contains "$BRANCH_BODY" ' = drop %[0-9]+.*ops \[%[0-9]+:value/borrow#0\]' "edge release block should drop the abandoned heap value"
assert_not_contains "$BRANCH_CONSUME_BODY" ' = dup ' "branch-local consuming uses should not force path-insensitive dup"
assert_not_contains "$BRANCH_CONSUME_BODY" ' = drop ' "branch-local consuming uses should not force path-insensitive drop"
assert_not_contains "$BRANCH_CONSUME_BODY" 'perceus\.edge\.drop' "branch-local consuming uses should not need edge drop blocks"
assert_contains "$BRANCH_CONSUME_BODY" 'call %[0-9]+\(%[0-9]+\).*value/consume#0' "branch-local true arm should consume the shared owner"
assert_order "$BRANCH_CONSUME_BODY" "branch-local false arm should also consume the shared owner without pre-branch duplication" \
  '^  bb3 match\.false:' \
  'call %[0-9]+\(%[0-9]+\).*value/consume#0' \
  'br bb1'
assert_not_contains "$BRANCH_RETURN_BODY" ' = drop ' "returned managed branch phi should not be dropped before return"
assert_not_contains "$BRANCH_RETURN_BODY" ' = dup ' "returned managed branch phi should not require duplication"
assert_order "$BRANCH_RETURN_BODY" "returned managed branch phi should flow directly to return" \
  '^  bb1 match\.cont:' \
  ' = phi \[bb2: %[0-9]+, bb3: %[0-9]+\]' \
  'return %[0-9]+'
assert_order "$PHI_CONSUME_BODY" "preexisting managed phi consume should release unselected owners on both incoming edges" \
  '^  bb[0-9]+ perceus\.edge\.drop:' \
  ' = drop %[0-9]+.*ops \[%[0-9]+:value/borrow#0\]' \
  '^  bb[0-9]+ perceus\.edge\.drop:' \
  ' = drop %[0-9]+.*ops \[%[0-9]+:value/borrow#0\]'
assert_order "$PHI_CONSUME_BODY" "selected managed phi should flow into the consuming call without duplication" \
  '^  bb1 match\.cont:' \
  ' = phi \[bb2: %[0-9]+, bb3: %[0-9]+\]' \
  'fn_ref \$consume_array' \
  'call %[0-9]+\(%[0-9]+\).*value/consume#0' \
  'return %[0-9]+'
assert_not_contains "$PHI_CONSUME_BODY" ' = dup ' "selected managed phi consumed once should not be duplicated"
assert_order "$PHI_DUP_BODY" "duplicated managed phi should release unselected owners on both incoming edges" \
  '^  bb[0-9]+ perceus\.edge\.drop:' \
  ' = drop %[0-9]+.*ops \[%[0-9]+:value/borrow#0\]' \
  '^  bb[0-9]+ perceus\.edge\.drop:' \
  ' = drop %[0-9]+.*ops \[%[0-9]+:value/borrow#0\]'
assert_order "$PHI_DUP_BODY" "duplicated managed phi should dup after the join before two consuming tuple fields" \
  '^  bb1 match\.cont:' \
  ' = phi \[bb2: %[0-9]+, bb3: %[0-9]+\]' \
  ' = dup %[0-9]+' \
  'construct\.tuple \{ %[0-9]+, %[0-9]+ \}.*ops \[%[0-9]+:field/consume#0, %[0-9]+:field/consume#1\]' \
  'return %[0-9]+'
assert_order "$PHI_BORROW_RETURN_BODY" "borrowed-return managed phi should release unselected owners on both incoming edges" \
  '^  bb[0-9]+ perceus\.edge\.drop:' \
  ' = drop %[0-9]+.*ops \[%[0-9]+:value/borrow#0\]' \
  '^  bb[0-9]+ perceus\.edge\.drop:' \
  ' = drop %[0-9]+.*ops \[%[0-9]+:value/borrow#0\]'
assert_order "$PHI_BORROW_RETURN_BODY" "borrowed managed phi should borrow for array_size and then return without dup" \
  '^  bb1 match\.cont:' \
  ' = phi \[bb2: %[0-9]+, bb3: %[0-9]+\]' \
  "$ARRAY_SIZE_MIR" \
  'return %[0-9]+'
assert_not_contains "$PHI_BORROW_RETURN_BODY" ' = dup ' "borrowed-return managed phi should not dup for array_size before return"
assert_order "$MATCH_BODY" "unused owned match result should be dropped in the continuation" \
  '^  bb1 match\.cont:' \
  ' = drop %[0-9]+' \
  'const\.int 0' \
  'return %[0-9]+'
assert_not_contains "$RANGE_BORROW_BODY" ' = drop ' "loop-borrowed returned array should not be dropped before return"
assert_order "$RANGE_REPLACE_BODY" "managed array_set should load and drop the overwritten slot" \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  'load_owned %[0-9]+ : Array' \
  'store %[0-9]+, %[0-9]+ : \(\).*ops \[%[0-9]+:value/borrow#0, %[0-9]+:element/consume#1\]' \
  ' = drop %[0-9]+'
assert_order "$RANGE_REPLACE_BODY" "returning a borrowed array cell should dup the extraction before dropping the container" \
  '^  bb4 loop\.after:' \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR" \
  ' = dup %[0-9]+' \
  ' = drop %[0-9]+' \
  'return %[0-9]+'
assert_not_contains "$EXTRACTED_BODY" ' = dup ' "stack-rooted extractions should not receive heap dup markers"
assert_not_contains "$EXTRACTED_BODY" ' = drop ' "stack-rooted extractions should not receive heap drop markers"
assert_not_contains "$TAIL_RETURN_BODY" ' = drop ' "tail-recursive managed transfer should not drop the transferred owner"
assert_not_contains "$TAIL_RETURN_BODY" ' = dup ' "tail-recursive managed transfer should not duplicate the transferred owner"
assert_contains "$TAIL_RETURN_BODY" 'call %[0-9]+\(%[0-9]+, %[0-9]+, %[0-9]+\).*value/consume#2' "tail-recursive call should consume the transferred owner"
assert_not_contains "$TAIL_RETURN_ENTRY_BODY" ' = drop ' "tail-recursive entry should not drop the value returned by the tail call"
assert_order "$TAIL_RETURN_ENTRY_BODY" "tail-recursive entry should pass the returned array into the tail call" \
  'fn_ref \$tail_return_array' \
  'fn_ref \$returned_array' \
  'call %[0-9]+\(%[0-9]+\).*Array' \
  'call %[0-9]+\(%[0-9]+, %[0-9]+, %[0-9]+\).*value/consume#2' \
  'return %[0-9]+'
assert_order "$LOCAL_PARTIAL_BODY" "borrowed closure extractions should keep the closure live until after the call" \
  'extract\.closure_env %[0-9]+' \
  'extract\.closure_fn %[0-9]+' \
  'call %[0-9]+.*callee/borrow#0.*value/borrow#0.*value/consume#1' \
  ' = drop %[0-9]+' \
  'return %[0-9]+'
assert_order "$ESCAPED_PARTIAL_BODY" "escaped closure extractions should also drop only after the call" \
  'extract\.closure_env %[0-9]+' \
  'extract\.closure_fn %[0-9]+' \
  'call %[0-9]+.*callee/borrow#0.*value/borrow#0.*value/consume#1' \
  ' = drop %[0-9]+' \
  'return %[0-9]+'
assert_order "$VARIANT_RETURN_BODY" "returned variant payload extraction should be owned before entering the match continuation" \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+' \
  'br bb4'
assert_not_contains "$VARIANT_DISCARD_BODY" ' = dup ' "discarded variant payload extraction should not be duplicated"
assert_order "$VARIANT_CONSUME_BODY" "consumed variant payload extraction should be duplicated before branch to consuming arm" \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+'
assert_contains "$VARIANT_CONSUME_BODY" 'call %[0-9]+\(%[0-9]+\).*value/consume#0' "variant payload consuming arm should pass extracted payload as an owned call operand"
assert_order "$VARIANT_LIST_HEAD_BODY" "variant list payload borrow chain should own borrows before leaving payload block" \
  'extract\.variant_payload %[0-9]+, Some#0' \
  'extract\.field %[0-9]+, 0' \
  'extract\.list_head %[0-9]+' \
  ' = dup %[0-9]+' \
  ' = drop %[0-9]+'
assert_contains "$VARIANT_LIST_HEAD_BODY" 'call %[0-9]+\(%[0-9]+\).*value/consume#0' "variant list payload consuming arm should consume the extracted head"
assert_order "$TUPLE_LET_MANAGED_BODY" "managed let destructure should dup the borrowed field before consuming it" \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+' \
  'call %[0-9]+.*value/consume#0'
assert_order "$VARIANT_RECORD_PAYLOAD_BODY" "managed variant record payload should dup the field before consuming it" \
  '^  bb7 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+' \
  'call %[0-9]+.*value/consume#0'
assert_order "$VARIANT_LITERAL_FAILURE_BODY" "literal-pattern failure should release the duplicated owned payload on the retry edge" \
  ' = dup %[0-9]+' \
  'const\.int 2' \
  'ieq %[0-9]+, %[0-9]+' \
  'perceus\.edge\.drop:' \
  ' = drop %[0-9]+'
assert_order "$LIST_RECORD_ARRAY_PAYLOAD_BODY" "managed array inside list-bound record should be consumed by the arm" \
  '^  bb4 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  'call %[0-9]+.*consume_array.*value/consume#0'
assert_order "$ARRAY_RECORD_LIST_PAYLOAD_BODY" "managed list inside array-bound record should be consumed by the arm" \
  '^  bb4 match\.arm\.0\.body:' \
  'extract\.field %[0-9]+, 0' \
  'call %[0-9]+.*consume_list.*value/consume#0'
assert_order "$HEAP_ARRAY_PATTERN_FAILURE_BODY" "heap array fallback should dup the borrowed payload before consuming it and then drop the scrutinee" \
  '^  bb8 match\.arm\.1\.body:' \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+' \
  'call %[0-9]+.*consume_array.*value/consume#0' \
  ' = drop %[0-9]+'
assert_order "$HEAP_LIST_PATTERN_FAILURE_BODY" "heap list fallback should dup the borrowed payload before consuming it and then drop the scrutinee" \
  '^  bb10 match\.arm\.1\.body:' \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+' \
  'call %[0-9]+.*consume_array.*value/consume#0' \
  ' = drop %[0-9]+'
assert_order "$DIRECT_RECURSIVE_LIST_BODY" "recursive list field match should dup borrowed tail and drop it on failure edges" \
  'extract\.field %[0-9]+, 1' \
  ' = dup %[0-9]+' \
  'perceus\.edge\.drop:' \
  ' = drop %[0-9]+'
assert_order "$DIRECT_RECURSIVE_ARRAY_BODY" "recursive array field match should dup borrowed array and drop it on failure edge" \
  'extract\.field %[0-9]+, 1' \
  ' = dup %[0-9]+' \
  'perceus\.edge\.drop:' \
  ' = drop %[0-9]+'
assert_order "$TENSOR_ADD_BODY" "TensorRef sum constructor should release consumed input arrays after storing them in the op payload" \
  'construct\.variant TensOpAdd#1\(%[0-9]+\)' \
  'construct\.array_literal \{ %[0-9]+ \}' \
  ' = drop %0' \
  ' = drop %1' \
  'return %[0-9]+'
assert_order "$TENSOR_OP_ADD_MATCH_BODY" "recursive sum match should dup both borrowed TensorRef payloads before body use" \
  'extract\.variant_payload %[0-9]+, TensOpAdd#1' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+' \
  'extract\.field %[0-9]+, 1' \
  ' = dup %[0-9]+'
assert_order "$TENSOR_OP_ADD_MATCH_BODY" "recursive sum match body should drop duplicated TensorRef borrows after array_size" \
  '^  bb4 match\.arm\.0\.body:' \
  "$ARRAY_SIZE_MIR" \
  "$ARRAY_SIZE_MIR" \
  'iadd %[0-9]+, %[0-9]+' \
  ' = drop %[0-9]+' \
  ' = drop %[0-9]+'
assert_order "$TENSOR_OP_ADD_CONSUME_BODY" "recursive sum consuming arm should dup selected TensorRef payload before consume" \
  'extract\.variant_payload %[0-9]+, TensOpAdd#1' \
  'extract\.field %[0-9]+, 0' \
  'extract\.field %[0-9]+, 0' \
  ' = dup %[0-9]+' \
  'br bb[0-9]+'
assert_order "$TENSOR_OP_ADD_CONSUME_BODY" "recursive sum consuming arm should pass selected TensorRef as owned call operand" \
  '^  bb4 match\.arm\.0\.body:' \
  'call %[0-9]+.*consume_tensor.*value/consume#0'
assert_order "$NESTED_EXTRACT_BODY" "nested borrowed extraction should keep the outer borrowed field live through element use" \
  'extract\.field %[0-9]+, 0' \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR" \
  'call %[0-9]+.*value/consume#0' \
  ' = drop %[0-9]+'
assert_order "$CLOSURE_USE_CELL_BODY" "closure env nested array extraction should keep captured array live through element use" \
  'extract\.field %[0-9]+, 0' \
  "$ARRAY_DATA_MIR" \
  "$ARRAY_PTR_OFFSET_MIR" \
  "$ARRAY_LOAD_MIR" \
  'call %[0-9]+.*value/consume#0' \
  ' = drop %[0-9]+'
assert_order "$ARRAY_OFFSET_BODY" "returned array_offset borrow should dup before dropping the source array" \
  "$ARRAY_VIEW_MIR" \
  ' = dup %[0-9]+' \
  ' = drop %[0-9]+' \
  'return %[0-9]+'
assert_order "$ARRAY_SUCC_BODY" "returned array_succ borrow should dup before dropping the source array" \
  "$ARRAY_VIEW_MIR" \
  ' = dup %[0-9]+' \
  ' = drop %[0-9]+' \
  'return %[0-9]+'

section "MIR to LLVM lowering"
assert_contains "$LLVM_IR" 'define .*@duplicated_array\.Int\.Tuple_Array_Int_Array_Int' "specialized duplicated_array should be lowered to LLVM"
assert_contains "$LLVM_IR" 'define .*@range_loop_replace_array_cell\.Int\.Array_Int' "range loop with managed array_set should be lowered to LLVM"
assert_contains "$LLVM_IR" 'loop\.cond:.*preds = %loop\.inc, %entry' "MIR loop CFG should lower to LLVM loop blocks"
assert_contains "$LLVM_IR" '%[A-Za-z0-9_.]+ = phi i32 \[ 0, %entry \], \[ %[A-Za-z0-9_.]+, %loop\.inc \]' "MIR_PHI should lower to LLVM phi"
assert_contains "$LLVM_IR" 'call void @__ylc_dup\(ptr' "MIR dup markers should lower to __ylc_dup calls"
assert_contains "$LLVM_IR" 'call void @__ylc_drop\(ptr' "MIR drop markers should lower to __ylc_drop calls"
assert_contains "$LLVM_IR" 'declare void @__ylc_dup\(ptr' "__ylc_dup hook should be declared"
assert_contains "$LLVM_IR" 'declare void @__ylc_drop\(ptr' "__ylc_drop hook should be declared"
assert_contains "$COROUTINE_LLVM_IR" 'define .*@coroutine_returned' "coroutine escape fixture should lower to LLVM"
assert_contains "$COROUTINE_LLVM_IR" 'call void @__ylc_dup\(ptr' "coroutine Perceus dup markers should lower to __ylc_dup calls"
assert_contains "$COROUTINE_LLVM_IR" 'call void @__ylc_drop\(ptr' "coroutine Perceus drop markers should lower to __ylc_drop calls"
assert_contains "$LLVM_IR" '^%YlcRcHeader = type \{ i32, i32 \}' "MIR heap-managed payloads should declare an RC header layout"
assert_order "$LLVM_IR" "heap array literal should allocate header plus payload and initialize RC header" \
  'define \{ i32, i32, ptr \} @returned_array' \
  'tail call ptr @malloc\(i32 ptrtoint \(ptr getelementptr \(\{ %YlcRcHeader, \[2 x i32\] \}, ptr null, i32 1\) to i32\)\)' \
  'getelementptr inbounds nuw \{ %YlcRcHeader, \[2 x i32\] \}, ptr %array\.data\.heap, i32 0, i32 0' \
  'store i32 1, ptr %rc\.count\.ptr' \
  'store i32 0, ptr %rc\.tag\.ptr' \
  'getelementptr inbounds nuw \{ %YlcRcHeader, \[2 x i32\] \}, ptr %array\.data\.heap, i32 0, i32 1' \
  'insertvalue \{ i32, i32, ptr \} \{ i32 2, i32 0, ptr undef \}, ptr %rc\.payload\.ptr, 2'
assert_order "$CLOSURE_LLVM_IR" "heap closure env should allocate header plus payload and initialize RC header" \
  'define %Closure @make_adder' \
  'tail call ptr @malloc\(i32 ptrtoint \(ptr getelementptr \(\{ %YlcRcHeader, \{ i32 \} \}, ptr null, i32 1\) to i32\)\)' \
  'getelementptr inbounds nuw \{ %YlcRcHeader, \{ i32 \} \}, ptr %closure\.env, i32 0, i32 0' \
  'store i32 1, ptr %rc\.count\.ptr' \
  'store i32 0, ptr %rc\.tag\.ptr' \
  'getelementptr inbounds nuw \{ %YlcRcHeader, \{ i32 \} \}, ptr %closure\.env, i32 0, i32 1' \
  'insertvalue %Closure undef, ptr %rc\.payload\.ptr, 1'
assert_order "$LLVM_IR" "managed array_set lowering should load old slot, store new value, then drop old payload" \
  'loop\.body:' \
  'extractvalue \{ i32, i32, ptr \} %array\.data, 2' \
  'getelementptr \{ i32, i32, ptr \}, ptr %extract\.field, i32 0' \
  'load \{ i32, i32, ptr \}, ptr %ptr\.offset' \
  'store \{ i32, i32, ptr \} %[A-Za-z0-9_.]+, ptr %ptr\.offset' \
  'call void @__ylc_drop\(ptr'
assert_order "$LLVM_IR" "returned extraction lowering should dup before dropping its source container" \
  'loop\.after:' \
  'extractvalue \{ i32, i32, ptr \} %array\.data, 2' \
  'getelementptr \{ i32, i32, ptr \}, ptr %extract\.field[0-9]*, i32 0' \
  'load \{ i32, i32, ptr \}, ptr %ptr\.offset[0-9]*' \
  'call void @__ylc_dup\(ptr' \
  'call void @__ylc_drop\(ptr' \
  'ret \{ i32, i32, ptr \}'
assert_order "$LLVM_IR" "preexisting managed phi consume should lower selected phi into consume call" \
  'define i32 @phi_select_preexisting_array_consume' \
  '^match\.cont:' \
  '%phi = phi \{ i32, i32, ptr \}' \
  'call i32 @consume_array\.Array_Int\.Int\(\{ i32, i32, ptr \} %phi\)'
assert_order "$LLVM_IR" "preexisting managed phi consume should lower unselected owner edge drops" \
  'define i32 @phi_select_preexisting_array_consume' \
  '^perceus\.edge\.drop:' \
  'call void @__ylc_drop\(ptr' \
  '^perceus\.edge\.drop1:' \
  'call void @__ylc_drop\(ptr'
assert_order "$LLVM_IR" "duplicated managed phi should lower RC dup before tuple insertvalues" \
  'define \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \} \} @phi_select_preexisting_array_duplicate' \
  '^match\.cont:' \
  '%phi = phi \{ i32, i32, ptr \}' \
  'call void @__ylc_dup\(ptr' \
  'insertvalue \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \} \} undef, \{ i32, i32, ptr \} %phi, 0' \
  'insertvalue \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \} \}'
assert_order "$LLVM_IR" "borrowed-return managed phi should lower array size read before returning phi" \
  'define \{ i32, i32, ptr \} @phi_select_preexisting_array_borrow_return' \
  '^match\.cont:' \
  '%phi = phi \{ i32, i32, ptr \}' \
  'extractvalue \{ i32, i32, ptr \} %phi, 0' \
  'ret \{ i32, i32, ptr \} %phi'
assert_contains "$EXTERN_LLVM_IR" '^declare i32 @abs\(i32' "specialized extern should declare the base C symbol"
assert_contains "$EXTERN_LLVM_IR" 'call i32 @abs\(i32 -1\)' "specialized extern calls should target the base C symbol"
assert_not_contains "$EXTERN_LLVM_IR" 'abs\.Int\.Int' "specialized extern MIR names should not become LLVM symbols"
assert_contains "$LLVM_SHAPE_IR" 'define i32 @tuple_shape\.Int\.Int' "LLVM shape fixture should lower tuple function"
assert_order "$LLVM_SHAPE_IR" "tuple construct/extract should lower to insertvalue/extractvalue" \
  'insertvalue \{ i32, i32 \} undef' \
  'insertvalue \{ i32, i32 \}' \
  'extractvalue \{ i32, i32 \}' \
  'extractvalue \{ i32, i32 \}'
assert_order "$LLVM_SHAPE_IR" "variant construct/tag/payload should lower to tagged struct operations" \
  'insertvalue \{ i8, i32 \}' \
  'extractvalue \{ i8, i32 \} %phi, 0' \
  'extractvalue \{ i8, i32 \} %phi, 1' \
  'insertvalue \{ i32 \} undef'
assert_order "$LLVM_SHAPE_IR" "list empty/cons/head/tail should lower to node allocation and field loads" \
  'alloca \{ i32, ptr \}' \
  'store ptr null' \
  'getelementptr inbounds nuw \{ i32, ptr \}.*i32 0, i32 0' \
  'load i32' \
  'getelementptr inbounds nuw \{ i32, ptr \}.*i32 0, i32 1' \
  'load ptr'
assert_contains "$LIST_MAP_TEST_LLVM_IR" 'define internal i1 @__ylc_mir_list_eq_int\(ptr' "list equality should lower to a typed helper"
assert_contains "$LIST_MAP_TEST_LLVM_IR" 'call i1 @__ylc_mir_list_eq_int\(ptr' "list equality calls should target the helper"
assert_contains "$LIST_MAP_TEST_LLVM_IR" 'define internal i1 @__ylc_mir_list_eq_num\(ptr' "double list equality should lower to a typed helper"
assert_contains "$LIST_MAP_TEST_LLVM_IR" 'sitofp i32 .* to double' "Double constructor should lower Int to Double casts"
assert_contains "$STD_LISTS_TEST_LLVM_IR" 'call ptr @map\.Fn_Int_Double\.List_Cons_Cons_Int_List\.List_Cons_Cons_Double_List\(ptr @"\$top\.<anonymous>\.[0-9]+"' "std Lists map lambda should pass a unique Double lambda function pointer"
assert_contains "$STD_LISTS_TEST_LLVM_IR" 'call ptr @map\.Fn_Int_Double\.List_Cons_Cons_Int_List\.List_Cons_Cons_Double_List\(ptr @builtin_Double\.Int\.Double' "std Lists map Double should pass the specialized builtin constructor"
assert_contains "$STD_LISTS_TEST_LLVM_IR" 'define double @"\$top\.<anonymous>\.[0-9]+"\(i32' "std Lists Double lambdas should lower as distinct double-returning functions"
assert_contains "$STD_LISTS_TEST_LLVM_IR" 'call i1 @contains\.Int\.List_Cons_Cons_Int_List\.Bool\(i32' "std Lists contains should lower recursive specialized calls"
assert_contains "$FIRST_CLASS_TEST_MIR" 'fn_ref \$sum\.Int\.Int\.Int' "first-class sum int ref should specialize in MIR"
assert_contains "$FIRST_CLASS_TEST_MIR" 'fn_ref \$sum\.Double\.Double\.Double' "first-class sum double ref should specialize in MIR"
assert_contains "$FIRST_CLASS_TEST_MIR" 'fn_ref \$builtin_op_add\.Int\.Int\.Int' "first-class builtin int ref should specialize in MIR"
assert_contains "$FIRST_CLASS_TEST_LLVM_IR" 'call i32 @proc\.Fn_Int_Int_Int\.Int\.Int\.Int\(ptr @sum\.Int\.Int\.Int' "first-class sum int ref should lower as a function pointer"
assert_contains "$FIRST_CLASS_TEST_LLVM_IR" 'call double @proc\.Fn_Double_Double_Double\.Double\.Double\.Double\(ptr @sum\.Double\.Double\.Double' "first-class sum double ref should lower as a function pointer"
assert_contains "$FIRST_CLASS_TEST_LLVM_IR" 'call i32 @proc\.Fn_Int_Int_Int\.Int\.Int\.Int\(ptr @builtin_op_add\.Int\.Int\.Int' "first-class builtin int ref should lower as a function pointer"
assert_not_contains "$FIRST_CLASS_TEST_LLVM_IR" 'ptr null' "first-class function refs should not lower to null pointers"
assert_order "$LLVM_SHAPE_IR" "closure construct/env/fn/field should lower to closure pair and env field load" \
  'insertvalue %Closure undef, ptr %closure\.env\.stack, 1' \
  'insertvalue %Closure .* ptr @closure_curried_add_[0-9]+\.Tuple_Int\.Int\.Int' \
  'extractvalue %Closure .* 1' \
  'extractvalue %Closure .* 0' \
  'getelementptr inbounds nuw \{ i32 \}, ptr %0, i32 0, i32 0' \
  'load i32'
assert_order "$LLVM_SHAPE_IR" "array literal/at/set should lower to data insert, store, and load" \
  'insertvalue \{ i32, i32, ptr \} \{ i32 2, i32 0, ptr undef \}, ptr %array\.data\.stack, 2' \
  'extractvalue \{ i32, i32, ptr \} %array\.data, 2' \
  'getelementptr i32, ptr %extract\.field, i32 0' \
  'store i32 .* ptr %ptr\.offset' \
  'extractvalue \{ i32, i32, ptr \} %array\.data, 2' \
  'getelementptr i32, ptr %extract\.field[0-9]*, i32 0' \
  'load i32, ptr %ptr\.offset[0-9]*'
assert_contains "$MATCH_DESTRUCTURE_LLVM_IR" 'define i32 @tuple_let_record_destructure\(i32 %0, i32 %1\)' "destructure LLVM fixture should lower tuple let-binding function"
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "tuple let destructure should lower nested records to insertvalue/extractvalue" \
  'define i32 @tuple_let_record_destructure' \
  'insertvalue \{ \{ i32 \}, \{ i32 \} \} undef' \
  'extractvalue \{ \{ i32 \}, \{ i32 \} \}' \
  'extractvalue \{ i32 \}' \
  'ret i32'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "plain tuple let destructure should lower to tuple insertvalue/extractvalue" \
  'define i32 @tuple_let_pair_destructure' \
  'insertvalue \{ i32, i32 \} undef' \
  'insertvalue \{ i32, i32 \}' \
  'extractvalue \{ i32, i32 \}' \
  'extractvalue \{ i32, i32 \}' \
  'ret i32'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "nested tuple let record destructure should lower through nested extractvalue" \
  'define i32 @tuple_let_nested_record_destructure' \
  'insertvalue \{ \{ \{ i32 \} \}, \{ \{ i32 \} \} \} undef' \
  'extractvalue \{ \{ \{ i32 \} \}, \{ \{ i32 \} \} \}' \
  'extractvalue \{ \{ i32 \} \}' \
  'extractvalue \{ i32 \}' \
  'ret i32'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "nested list record destructure should lower nested field extractvalue in the arm" \
  'define i32 @nested_record_list_match' \
  'extractvalue \{ \{ i32 \} \} %list\.head, 0' \
  'extractvalue \{ i32 \} %[A-Za-z0-9_.]+, 0'
assert_contains "$MATCH_DESTRUCTURE_LLVM_IR" 'load \{ \{ i32 \} \}, ptr %list\.head\.ptr' "nested list record destructure should lower list head loads"
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "nested array record destructure should lower nested field extractvalue in the arm" \
  'define i32 @nested_record_array_match' \
  'extractvalue \{ \{ i32 \} \} %ptr\.load, 0' \
  'extractvalue \{ i32 \} %[A-Za-z0-9_.]+, 0'
assert_contains "$MATCH_DESTRUCTURE_LLVM_IR" 'load \{ \{ i32 \} \}, ptr %ptr\.offset[0-9]*' "nested array record destructure should lower array element loads"
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "variant record payload destructure should lower payload extraction" \
  'define i32 @variant_record_payload_match' \
  'extractvalue \{ i8, \{ \{ i32, i32, ptr \}, i32 \} \} %phi, 1' \
  'insertvalue \{ \{ \{ i32, i32, ptr \}, i32 \} \} undef' \
  'extractvalue \{ \{ \{ i32, i32, ptr \}, i32 \} \}'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "variant record payload arm should lower RC dup before consume" \
  'define i32 @variant_record_payload_match' \
  'call void @__ylc_dup\(ptr' \
  'call i32 @consume_array\.Array_Int\.Int'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "literal-pattern failure should lower retry-edge drop after the failed compare" \
  'define i32 @variant_tuple_literal_failure_match' \
  'call void @__ylc_dup\(ptr' \
  'icmp eq i32 %[A-Za-z0-9_.]+, 2' \
  'br i1 %[A-Za-z0-9_.]+, label %match\.arm\.0\.body, label %perceus\.edge\.drop' \
  '^perceus\.edge\.drop:' \
  'call void @__ylc_drop\(ptr' \
  'br label %match\.arm\.1\.test'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "array-pattern fallback should lower second length test before one-element payload consume" \
  'define i32 @array_pattern_failure_then_payload_match' \
  'extractvalue \{ i32, i32, ptr \} %array\.data, 0' \
  'icmp eq i32 %extract\.field, 2' \
  'extractvalue \{ i32, i32, ptr \} %array\.data, 0' \
  'icmp eq i32 %extract\.field[0-9]+, 1' \
  'load \{ \{ i32, i32, ptr \} \}, ptr %ptr\.offset[0-9]*' \
  'call i32 @consume_array\.Array_Int\.Int'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "list record array payload match should lower list loads into record field extraction" \
  'define i32 @list_record_array_payload_match' \
  'load \{ \{ i32, i32, ptr \} \}, ptr %list\.head\.ptr' \
  'load ptr, ptr %list\.tail\.ptr' \
  'extractvalue \{ \{ i32, i32, ptr \} \} %list\.head, 0' \
  'call i32 @consume_array\.Array_Int\.Int'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "list record array payload match should expose a match continuation phi" \
  'define i32 @list_record_array_payload_match' \
  '^match\.cont:' \
  '%phi = phi i32'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "array record list payload match should length-check before loading the record payload" \
  'define i32 @array_record_list_payload_match' \
  'extractvalue \{ i32, i32, ptr \} %array\.data, 0' \
  'icmp eq i32 %extract\.field, 1'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "array record list payload match should load the record payload" \
  'define i32 @array_record_list_payload_match' \
  'extractvalue \{ i32, i32, ptr \} %array\.data, 2' \
  'getelementptr \{ ptr \}, ptr %extract\.field[0-9]*, i32 0' \
  'load \{ ptr \}, ptr %ptr\.offset[0-9]*' \
  'br label %match\.arm\.0\.body'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "array record list payload match should extract and consume the list field in the arm" \
  'define i32 @array_record_list_payload_match' \
  '^match\.arm\.0\.body:' \
  'extractvalue \{ ptr \} %ptr\.load, 0' \
  'call i32 @consume_list\.List_Cons_Cons_Array_Int_List\.Int'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "variant first-field failure should compare before extracting the managed payload field" \
  'define i32 @variant_first_field_failure_payload_match' \
  'extractvalue \{ i8, \{ i32, \{ i32, i32, ptr \} \} \} %phi, 1' \
  'extractvalue \{ i32, \{ i32, i32, ptr \} \} %extract\.field, 0' \
  'icmp eq i32 %extract\.field[0-9]+, 2' \
  'br i1 %eq, label %match\.tuple\.[0-9]+\.1, label %match\.arm\.1\.test' \
  'extractvalue \{ i32, \{ i32, i32, ptr \} \} %extract\.field, 1' \
  'call void @__ylc_dup\(ptr'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "heap array pattern failure should lower retry-edge drop after failed length tests" \
  'define i32 @heap_array_pattern_failure_then_payload_match' \
  'extractvalue \{ i32, i32, ptr \} %call, 0' \
  'icmp eq i32 %extract\.field, 2' \
  'extractvalue \{ i32, i32, ptr \} %call, 0' \
  'icmp eq i32 %extract\.field[0-9]+, 1' \
  'br i1 %eq[0-9]*, label %match\.array\.bb5, label %perceus\.edge\.drop' \
  '^perceus\.edge\.drop:' \
  'call void @__ylc_drop\(ptr'
assert_order "$MATCH_DESTRUCTURE_LLVM_IR" "heap list pattern failure should lower tail dup and retry-edge drop" \
  'define i32 @heap_list_pattern_failure_then_payload_match' \
  'load ptr, ptr %list\.tail\.ptr[0-9]*' \
  'call void @__ylc_dup\(ptr %list\.tail[0-9]*\)' \
  'icmp eq ptr %list\.tail[0-9]*, null' \
  '^perceus\.edge\.drop[0-9]*:' \
  'call void @__ylc_drop\(ptr %list\.tail[0-9]*\)'
assert_contains "$RECURSIVE_DESTRUCTURE_LLVM_IR" '^%DirectNode = type \{ i32, ptr \}' "recursive list record should lower to named LLVM struct"
assert_contains "$RECURSIVE_DESTRUCTURE_LLVM_IR" '^%DirectArrayNode = type \{ i32, \{ i32, i32, ptr \} \}' "recursive array record should lower to named LLVM struct"
assert_order "$RECURSIVE_DESTRUCTURE_LLVM_IR" "recursive list constructor should lower to list node stores and record insertvalue" \
  'define %DirectNode @direct_pair' \
  'getelementptr \(\{ %YlcRcHeader, \{ %DirectNode, ptr \} \}, ptr null, i32 1\)' \
  'store i32 1, ptr %rc\.count\.ptr' \
  'store i32 0, ptr %rc\.tag\.ptr' \
  'store %DirectNode %1, ptr %list\.head\.ptr' \
  'store %DirectNode %0, ptr %list\.head\.ptr' \
  'insertvalue %DirectNode \{ i32 3, ptr undef \}, ptr %rc\.payload\.ptr[0-9]*, 1'
assert_order "$RECURSIVE_DESTRUCTURE_LLVM_IR" "recursive array constructor should lower to array storage of DirectArrayNode" \
  'define %DirectArrayNode @direct_array_pair' \
  'getelementptr \(\{ %YlcRcHeader, \[2 x %DirectArrayNode\] \}, ptr null, i32 1\)' \
  'store i32 1, ptr %rc\.count\.ptr' \
  'store i32 0, ptr %rc\.tag\.ptr' \
  'store %DirectArrayNode %0, ptr %array\.item\.ptr' \
  'store %DirectArrayNode %1, ptr %array\.item\.ptr' \
  'insertvalue %DirectArrayNode \{ i32 3, \{ i32, i32, ptr \} undef \}, \{ i32, i32, ptr \} %array\.data, 1'
assert_order "$RECURSIVE_DESTRUCTURE_LLVM_IR" "recursive list match should lower list head loads and RC edge drops" \
  'define i32 @direct_recursive_record_list_match' \
  'extractvalue %DirectNode %[A-Za-z0-9_.]+, 1' \
  'load %DirectNode, ptr %list\.head\.ptr' \
  'call void @__ylc_dup\(ptr %list\.tail' \
  'perceus\.edge\.drop:' \
  'call void @__ylc_drop\(ptr'
assert_order "$RECURSIVE_DESTRUCTURE_LLVM_IR" "recursive array match should lower element loads and RC edge drop" \
  'define i32 @direct_recursive_record_array_match' \
  'extractvalue %DirectArrayNode %[A-Za-z0-9_.]+, 1' \
  'extractvalue \{ i32, i32, ptr \} %extract\.field, 2' \
  'load %DirectArrayNode, ptr %ptr\.offset' \
  'perceus\.edge\.drop:' \
  'call void @__ylc_drop\(ptr'
assert_order "$RECURSIVE_DESTRUCTURE_LLVM_IR" "TensorRef sum constructor should lower payload tuple and tagged storage" \
  'define \{ i32, i32, ptr \} @tensor_add' \
  'insertvalue \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \} \} undef' \
  'insertvalue \{ \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \} \} \} undef' \
  'insertvalue \{ i8, \[32 x i8\] \} \{ i8 1, \[32 x i8\] undef \}' \
  'store \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \}, \{ i32, i32, ptr \}, \{ i8, \[32 x i8\] \} \}'
assert_order "$RECURSIVE_DESTRUCTURE_LLVM_IR" "TensorRef op projection should lower array load and op extractvalue" \
  'define \{ i8, \[32 x i8\] \} @tensor_op' \
  'load \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \}, \{ i32, i32, ptr \}, \{ i8, \[32 x i8\] \} \}' \
  'extractvalue \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \}, \{ i32, i32, ptr \}, \{ i8, \[32 x i8\] \} \} %ptr\.load, 3'
assert_order "$RECURSIVE_DESTRUCTURE_LLVM_IR" "TensorRef sum match should lower array_size uses and drop duplicated payloads" \
  'define i32 @tensor_op_add_payload_match' \
  '^match\.arm\.0\.body:' \
  'extractvalue \{ i32, i32, ptr \} %extract\.field[0-9]+, 0' \
  'extractvalue \{ i32, i32, ptr \} %extract\.field[0-9]+, 0' \
  'add i32' \
  'call void @__ylc_drop\(ptr' \
  'call void @__ylc_drop\(ptr' \
  'br label %match\.cont'
assert_order "$RECURSIVE_DESTRUCTURE_LLVM_IR" "TensorRef sum match should lower payload unpack and RC dup" \
  'define i32 @tensor_op_add_payload_match' \
  'extractvalue \{ i8, \[32 x i8\] \} %call[0-9]*, 1' \
  'load \{ \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \} \} \}, ptr %union_cast_temp' \
  'extractvalue \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \} \} %extract\.field, 0' \
  'call void @__ylc_dup\(ptr' \
  'extractvalue \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \} \} %extract\.field, 1' \
  'call void @__ylc_dup\(ptr'
assert_order "$RECURSIVE_DESTRUCTURE_LLVM_IR" "TensorRef sum consuming arm should lower owned consume call" \
  'define i32 @tensor_op_add_payload_consume' \
  '^match\.arm\.0\.body:' \
  'call i32 @consume_tensor.*\(\{ i32, i32, ptr \} %extract\.field[0-9]+\)'
assert_order "$RECURSIVE_DESTRUCTURE_LLVM_IR" "TensorRef sum consuming arm should lower RC dup for selected payload" \
  'define i32 @tensor_op_add_payload_consume' \
  'extractvalue \{ \{ i32, i32, ptr \}, \{ i32, i32, ptr \} \} %extract\.field, 0' \
  'call void @__ylc_dup\(ptr'

section "MIR test mode"
if [ "$FIB_TEST_STATUS" -eq 0 ]; then
  pass "01_fib should pass in MIR --test mode"
else
  fail "01_fib should pass in MIR --test mode"
fi
if [ "$LIST_MAP_TEST_STATUS" -eq 0 ]; then
  pass "04_list_map should pass in MIR --test mode"
else
  fail "04_list_map should pass in MIR --test mode"
fi
if [ "$FIRST_CLASS_TEST_STATUS" -eq 0 ]; then
  pass "05_1st_class_fns should pass in MIR --test mode"
else
  fail "05_1st_class_fns should pass in MIR --test mode"
fi
if [ "$STD_LISTS_TEST_STATUS" -eq 0 ]; then
  pass "std/Lists should pass in MIR --test mode"
else
  fail "std/Lists should pass in MIR --test mode"
fi
assert_contains "$FIB_TEST_MIR" '^fn \$module_init\(\).* -> \(\) @none' "test mode should keep top-level initialization separate from test entry"
assert_contains "$FIB_TEST_MIR" '^fn \$top\(\).* -> Bool @owned' "test mode should expose a boolean MIR entry point"
assert_contains "$FIB_TEST_MIR" '^extern fn _report_test_result\(%0 arg0: Ptr @borrow, %1 arg1: Bool @borrow\) -> \(\) @none;' "test-mode top should declare the per-test reporter"
assert_contains "$FIB_TEST_MIR" '^extern fn _report_test_totals\(%0 arg0: Int @borrow, %1 arg1: Int @borrow\) -> \(\) @none;' "test-mode top should declare the totals reporter"
assert_contains "$FIB_TEST_MIR" 'call %[0-9]+( as \$fib\.Int\.Int)?\(%[0-9]+\).*: Int' "test-mode top should call module-level functions"
assert_contains "$FIB_TEST_MIR" 'call %[0-9]+\(%[0-9]+, %[0-9]+\).*: \(\)' "test-mode top should report each test result"
assert_contains "$FIB_TEST_MIR" '^    %[0-9]+ = phi \[bb[0-9]+: %[0-9]+, bb[0-9]+: %[0-9]+\].*Bool' "test-mode top should combine test bindings through boolean phis"
assert_contains "$FIB_TEST_OUT" 'test_fib20' "MIR --test mode should print individual test names"
assert_contains "$FIB_TEST_OUT" '5 / 5 passed' "MIR --test mode should print test totals"
assert_contains "$LIST_MAP_TEST_OUT" 'test_map_plus' "MIR --test mode should print list-map test name"
assert_contains "$LIST_MAP_TEST_OUT" 'test_map_to_double' "MIR --test mode should print list-map Double constructor test name"
assert_contains "$LIST_MAP_TEST_OUT" '2 / 2 passed' "MIR --test mode should pass list-map totals"
assert_contains "$FIRST_CLASS_TEST_OUT" 'test_1st_class_binop_double' "MIR --test mode should print first-class function test name"
assert_contains "$FIRST_CLASS_TEST_OUT" '4 / 4 passed' "MIR --test mode should pass first-class function totals"
assert_contains "$STD_LISTS_TEST_OUT" 'test_map_to_double' "MIR --test mode should print std Lists lambda Double map test"
assert_contains "$STD_LISTS_TEST_OUT" 'test_map_to_double2' "MIR --test mode should print std Lists builtin Double map test"
assert_contains "$STD_LISTS_TEST_OUT" 'test_contains' "MIR --test mode should print std Lists contains test"
assert_contains "$STD_LISTS_TEST_OUT" 'test_not_contains' "MIR --test mode should print std Lists negative contains test"
assert_contains "$STD_LISTS_TEST_OUT" '9 / 9 passed' "MIR --test mode should pass std Lists totals"


echo
if [ "$FAILS" -eq 0 ]; then
  echo "${GREEN}✓${NC} ${PASSES}/${CHECKS} MIR pipeline checks passed"
else
  echo "${RED}✗${NC} ${FAILS}/${CHECKS} MIR pipeline checks failed"
  echo "  artifacts: $ARTIFACT_DIR"
  exit 1
fi
