#include "eval_function.h"
#include "serde.h"
#include <stdlib.h>
#include <string.h>
Value eval(Ast *ast, LangCtx *ctx_fn_t);

Value fn_call(Function fn, Value *input_vals, LangCtx *ctx) {

  int stack_ptr = fn.is_recursive_ref ? fn.scope_ptr : fn.scope_ptr + 1;
  // int stack_ptr = fn.scope_ptr + 1;

  ht *fn_scope = ctx->stack + stack_ptr;

  for (int i = 0; i < fn.len; i++) {
    ObjString param_id = fn.params[i];
    ht_set_hash(fn_scope, param_id.chars, param_id.hash, (input_vals + i));
  }

  if (fn.fn_name != NULL) {
    const char *fn_name = fn.fn_name;
    uint64_t hash = hash_string(fn_name, strlen(fn_name));

    Value recursive_ref = {VALUE_FN, .value = {.function = fn}};
    recursive_ref.value.function.is_recursive_ref = true;
    recursive_ref.value.function.num_partial_args = 0;
    ht_set_hash(fn_scope, fn_name, hash, &recursive_ref);
  }

  LangCtx new_ctx = {
      .stack = ctx->stack,
      .stack_ptr = stack_ptr,
      .val_bind = ctx->val_bind,
  };

  Value return_val = eval(fn.body, &new_ctx);
  // free(input_vals);
  return return_val;
}

static Value fn_application(Function func, int num_input, Value *input_vals,
                            LangCtx *ctx) {
  int len = func.len;
  if (num_input == len) {
    if (func.is_recursive_ref) {
      // for (int i = 0; i < num_input; i++) {
      //   print_value(input_vals + i);
      // }
      // printf("recursive call?\n");
    }
    Value res = fn_call(func, input_vals, ctx);
    // free(input_vals);
    return res;
  }

  if (num_input + func.num_partial_args > func.len) {
    fprintf(stderr, "Error: too many arguments to %s, expected %d got %d",
            func.fn_name, func.len, num_input + func.num_partial_args);
    return VOID;
  }

  if (num_input + func.num_partial_args == func.len) {
    // copy input_vals into func's partial_args
    //
    for (int i = 0; i < num_input; i++) {
      func.partial_args[i + func.num_partial_args] = input_vals[i];
    }
    // free(input_vals);
    Value res = fn_call(func, func.partial_args, ctx);
    return res;
  }

  if (num_input + func.num_partial_args < func.len) {
    // copy input vals to a new version of partial_args
    // along with original partial_args
    // and set num_partial_args to num_input + func.num_partial_args
    Value *partial_args = malloc(sizeof(Value) * len);

    // printf("num partial args %d\n", func.num_partial_args);
    // print_value(partial_args + i);
    for (int i = 0; i < func.num_partial_args; i++) {
      partial_args[i] = func.partial_args[i];
    }

    for (int i = 0; i < num_input; i++) {
      partial_args[func.num_partial_args + i] = input_vals[i];
    }

    Value res =
        (Value){VALUE_FN,
                {.function = {
                     .len = len,
                     .body = func.body,
                     .params = func.params,
                     .fn_name = func.fn_name,
                     .scope_ptr = func.scope_ptr,
                     .num_partial_args = func.num_partial_args + num_input,
                     .partial_args = partial_args,

                 }}};
    printf("res num partial args %d ", res.value.function.num_partial_args);
    print_value(&res);
    return res;
  }
  return VOID;
}

static Value native_fn_call(NativeFn fn, Value *input_vals) {
  int len = fn.len;
  return fn.handle(input_vals);
}

static Value native_fn_application(NativeFn func, int num_input,
                                   Value *input_vals) {
  int len = func.len;
  if (num_input == len) {
    Value res = native_fn_call(func, input_vals);
    // free(input_vals);
    return res;
  }

  if (num_input + func.num_partial_args > func.len) {
    fprintf(stderr, "Error: too many arguments to expected %d got %d", func.len,
            num_input + func.num_partial_args);
    return VOID;
  }

  if (num_input + func.num_partial_args == func.len) {
    // copy input_vals into func's partial_args
    for (int i = 0; i < num_input; i++) {
      func.partial_args[i + func.num_partial_args] = input_vals[i];
    }
    // free(input_vals);
    Value res = native_fn_call(func, func.partial_args);
    return res;
  }

  if (num_input + func.num_partial_args < func.len) {
    // copy input vals to a new version of partial_args
    // along with original partial_args
    // and set num_partial_args to num_input + func.num_partial_args
    Value *partial_args = malloc(sizeof(Value) * len);

    for (int i = 0; i < func.num_partial_args; i++) {
      partial_args[i] = func.partial_args[i];
    }

    for (int i = 0; i < num_input; i++) {
      partial_args[func.num_partial_args + i] = input_vals[i];
    }

    Value res =
        (Value){VALUE_FN,
                {.native_fn = {
                     .handle = func.handle,
                     .len = len,
                     .num_partial_args = func.num_partial_args + num_input,
                     .partial_args = partial_args,

                 }}};

    return res;
  }
  return VOID;
}

Value eval_application(Ast *ast, LangCtx *ctx) {
  // print_ast(ast);

  Value func_val = eval(ast->data.AST_APPLICATION.function, ctx);

  int app_len = ast->data.AST_APPLICATION.len;

  Value *input_vals;
  if (func_val.type == VALUE_FN && func_val.value.function.is_recursive_ref) {
    input_vals = func_val.value.function.partial_args;
  } else {
    input_vals = malloc(sizeof(Value) * app_len);
  }

  for (int i = 0; i < app_len; i++) {
    Value arg = eval(ast->data.AST_APPLICATION.args + i, ctx);
    input_vals[i] = arg;
  }

  if (func_val.type == VALUE_FN) {
    Value res =
        fn_application(func_val.value.function, app_len, input_vals, ctx);
    // free(input_vals);
    return res;
  }
  return native_fn_application(func_val.value.native_fn, app_len, input_vals);
}

Value eval_lambda_declaration(Ast *ast, LangCtx *ctx) {
  Value val;

  int len = ast->data.AST_LAMBDA.len;
  val.type = VALUE_FN;
  val.value.function.len = len;
  val.value.function.params = ast->data.AST_LAMBDA.params;
  val.value.function.fn_name = ast->data.AST_LAMBDA.fn_name.chars;
  val.value.function.body = ast->data.AST_LAMBDA.body;
  val.value.function.scope_ptr = ctx->stack_ptr;
  val.value.function.partial_args = malloc(sizeof(Value) * len);
  val.value.function.num_partial_args = 0;

  return val;
}
