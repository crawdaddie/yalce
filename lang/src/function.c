#include "function.h"
#include <stdlib.h>
#include <string.h>

Value eval(Ast *ast, ht *stack, int stack_ptr);

static Value call_function(Function fn, ht *stack) {
  int stack_ptr = fn.scope_ptr + 1;

  ht *fn_scope = stack + stack_ptr;

  for (int i = 0; i < fn.len; i++) {
    ObjString param_id = fn.params[i];
    ht_set_hash(fn_scope, param_id.chars, param_id.hash, (fn.partial_args + i));
  }

  if (fn.fn_name != NULL) {

    const char *fn_name = fn.fn_name;
    uint64_t hash = hash_string(fn_name, strlen(fn_name));

    Value recursive_ref = {VALUE_RECURSIVE_REF, .value = {.recursive_ref = fn}};
    ht_set_hash(fn_scope, fn_name, hash, &recursive_ref);
  }
  Value return_val = eval(fn.body, stack, stack_ptr);
  ht_reinit(fn_scope);
  return return_val;
}

static Value call_native_function(NativeFn fn) {
  int len = fn.len;
  Value *input_vals = fn.partial_args;
  return fn.handle(len, input_vals);
}

static Value partial_fn_application(Function func, int app_len, Ast **args,
                                    ht *stack, int stack_ptr) {

  int len = func.len;
  if (func.partial_args == NULL) {
    func.partial_args = malloc(sizeof(Value) * len);
  }
  int num_partial_args = func.num_partial_args;
  for (int i = func.num_partial_args; i < num_partial_args + app_len; i++) {
    *(func.partial_args + i) = eval(args[i], stack, stack_ptr);
    func.num_partial_args++;
  }
  return (Value){VALUE_FN, {.function = func}};
}

static Value partial_native_fn_application(NativeFn func, int app_len,
                                           Ast **args, ht *stack,
                                           int stack_ptr) {

  int len = func.len;
  if (func.partial_args == NULL) {
    func.partial_args = malloc(sizeof(Value) * len);
  }
  int num_partial_args = func.num_partial_args;
  for (int i = func.num_partial_args; i < num_partial_args + app_len; i++) {
    *(func.partial_args + i) = eval(args[i], stack, stack_ptr);
    func.num_partial_args++;
  }
  return (Value){VALUE_NATIVE_FN, {.native_fn = func}};
}

static Value fn_application(Function func, int app_len, Ast **args, ht *stack,
                            int stack_ptr) {

  int len = func.len;
  if (app_len + func.num_partial_args < len) {
    return partial_fn_application(func, app_len, args, stack, stack_ptr);
  }

  if (app_len == len) {
    Value *arg_vals = malloc(sizeof(Value) * len);
    for (int i = 0; i < len; i++) {
      *(arg_vals + i) = eval(args[i], stack, stack_ptr);
    }
    func.partial_args = arg_vals;
    Value val = call_function(func, stack);
    free(arg_vals);
    return val;
  }

  if (app_len + func.num_partial_args == len && func.partial_args != NULL) {

    for (int i = 0; i < app_len; i++) {
      *(func.partial_args + (func.num_partial_args + i)) =
          eval(args[i], stack, stack_ptr);
    }
    Value val = call_function(func, stack);
    return val;
  }
  return (Value){VALUE_VOID};
}

static Value native_fn_application(NativeFn func, int app_len, Ast **args,
                                   ht *stack, int stack_ptr) {

  int len = func.len;
  if (app_len + func.num_partial_args < len) {
    return partial_native_fn_application(func, app_len, args, stack, stack_ptr);
  }

  if (app_len == len) {
    Value *arg_vals = malloc(sizeof(Value) * len);
    for (int i = 0; i < len; i++) {
      *(arg_vals + i) = eval(args[i], stack, stack_ptr);
    }
    func.partial_args = arg_vals;
    Value val = call_native_function(func);
    free(arg_vals);
    return val;
  }

  if (app_len + func.num_partial_args == len && func.partial_args != NULL) {

    for (int i = 0; i < app_len; i++) {
      *(func.partial_args + (func.num_partial_args + i)) =
          eval(args[i], stack, stack_ptr);
    }
    Value val = call_native_function(func);
    return val;
  }

  return (Value){VALUE_VOID};
}

Value eval_application(Ast *ast, ht *stack, int stack_ptr) {
  Value func_ = eval(ast->data.AST_APPLICATION.function, stack, stack_ptr);

  if (func_.type == VALUE_FN) {
    Function func = func_.value.function;

    int app_len = ast->data.AST_APPLICATION.len;
    return fn_application(func, app_len, ast->data.AST_APPLICATION.args, stack,
                          stack_ptr);
  }

  if (func_.type == VALUE_NATIVE_FN) {

    NativeFn func = func_.value.native_fn;

    int app_len = ast->data.AST_APPLICATION.len;
    return native_fn_application(func, app_len, ast->data.AST_APPLICATION.args,
                                 stack, stack_ptr);
  }
  return (Value){VALUE_VOID};
}
