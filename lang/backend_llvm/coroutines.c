#include "coroutines.h"
#include "function.h"
#include "match.h"
#include "serde.h"
#include "symbols.h"
#include "tuple.h"
#include "types.h"
#include "llvm-c/Core.h"
#include <stdlib.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMTypeRef param_struct_type(Type *fn_type, int fn_len, Type *param_obj_type,
                              Type *return_type, TypeEnv *env,
                              LLVMModuleRef module) {

  LLVMTypeRef llvm_param_types[fn_len];

  Type **contained = talloc(sizeof(Type *) * fn_len);

  for (int i = 0; i < fn_len; i++) {
    Type *t = fn_type->data.T_FN.from;
    contained[i] = t;

    llvm_param_types[i] = type_to_llvm_type(t, env, module);

    if (t->kind == T_FN) {
      llvm_param_types[i] = LLVMPointerType(llvm_param_types[i], 0);
    } else if (is_pointer_type(t)) {
      llvm_param_types[i] = LLVMPointerType(
          type_to_llvm_type(t->data.T_CONS.args[0], env, module), 0);
    }

    fn_type = fn_type->data.T_FN.to;
  }
  *return_type = *(fn_type->data.T_FN.to);
  param_obj_type->kind = T_CONS;
  param_obj_type->data.T_CONS.name = TYPE_NAME_TUPLE;
  param_obj_type->data.T_CONS.args = contained;
  param_obj_type->data.T_CONS.num_args = fn_len;

  return LLVMStructType(llvm_param_types, fn_len, 0);
}

typedef struct coroutine_ctx_t {
  LLVMValueRef switch_val;
  LLVMBasicBlockRef *block_refs;
  LLVMTypeRef ret_option_type;
  int num_branches;
  int current_branch;
} coroutine_ctx_t;

static coroutine_ctx_t _coroutine_ctx = {};
LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  print_ast(ast->data.AST_YIELD.expr);
  print_type(ast->data.AST_YIELD.expr->md);

  LLVMValueRef val = codegen(ast->data.AST_YIELD.expr, ctx, module, builder);
  LLVMValueRef ret_opt = LLVMGetUndef(_coroutine_ctx.ret_option_type);

  ret_opt =
      LLVMBuildInsertValue(builder, ret_opt, LLVMConstInt(LLVMInt8Type(), 0, 0),
                           0, "insert Some tag");

  ret_opt = LLVMBuildInsertValue(builder, ret_opt, val, 1, "insert Some Value");
  LLVMBuildRet(builder, ret_opt);

  _coroutine_ctx.current_branch++;
  LLVMPositionBuilderAtEnd(
      builder, _coroutine_ctx.block_refs[_coroutine_ctx.current_branch]);
  return ret_opt;
}

void add_recursive_cr_def_ref(ObjString fn_name, LLVMValueRef func,
                              Type *fn_type,
                              coroutine_generator_symbol_data_t symbol_data,
                              JITLangCtx *fn_ctx) {

  JITSymbol *sym =
      new_symbol(STYPE_COROUTINE_GENERATOR, fn_type, func, LLVMTypeOf(func));
  sym->symbol_data.STYPE_COROUTINE_GENERATOR = symbol_data;

  ht *scope = fn_ctx->stack + fn_ctx->stack_ptr;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
}
/**
 * given a coroutine generator function
 * return a function that implements a switch statement with a case for each
 * 'yield' and takes as arguments the function's normal args, + any local
 * variables used in the yielded expressions + an integer to use for the switch
 * let f = fn a b c ->
 *   yield a;
 *   yield b;
 *   yield c;
 *   yield 4;
 *   yield 5
 * ;;

 * we generate a function like
 * (pseudo-code)
 * ```
 *
 * let gen = fn (a, b, c) i ->
 *
 *  switch i
 *
 *  case 0:
 *
 *    return Some a; # yield 1 -> (0, a) -- tagged union
 *
 *  case 1:
 *
 *    return Some b; # yield 2 -> (0, b)
 *
 *  case 2:
 *
 *    return Some c; # yield 3 -> (0, c)
 *
 *  case 3:
 *
 *    return Some 4; # yield 4 -> (0, 4)
 *
 *  case 4:
 *
 *    return Some 5; # yield 5 -> (0, 5)
 *
 *  default:
 *
 *    return None;   # end coroutine -> (1, _)
 *
 *  ```
 */
LLVMValueRef codegen_coroutine_generator(
    Ast *ast, coroutine_generator_symbol_data_t symbol_data, JITLangCtx *ctx,
    LLVMModuleRef module, LLVMBuilderRef builder) {
  coroutine_ctx_t prev_cr_ctx = _coroutine_ctx;
  int num_yields = ast->data.AST_LAMBDA.num_yields;

  _coroutine_ctx =
      (coroutine_ctx_t){.num_branches = num_yields + 1, .current_branch = 0};

  ObjString fn_name = ast->data.AST_LAMBDA.fn_name;
  bool is_anon = false;

  if (fn_name.chars == NULL) {
    is_anon = true;
  }

  Type *fn_type = ast->md;
  size_t args_len = ast->data.AST_LAMBDA.len;

  _coroutine_ctx.ret_option_type = symbol_data.llvm_ret_option_type;

  LLVMValueRef func = LLVMAddFunction(
      module, !is_anon ? fn_name.chars : "anonymous_coroutine_def",
      symbol_data.def_fn_type);

  LLVMSetLinkage(func, LLVMExternalLinkage);

  JITLangCtx fn_ctx = ctx_push(*ctx);
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef _block_refs[_coroutine_ctx.num_branches];
  _coroutine_ctx.block_refs = _block_refs;
  for (int i = 0; i < num_yields; i++) {
    _coroutine_ctx.block_refs[i] = LLVMAppendBasicBlock(func, "yield_case");
  }
  _coroutine_ctx.block_refs[num_yields] = LLVMAppendBasicBlock(func, "default");
  LLVMPositionBuilderAtEnd(builder, entry);

  if (!is_anon) {
    add_recursive_cr_def_ref(fn_name, func, fn_type, symbol_data, &fn_ctx);
  }

  LLVMValueRef param_val = LLVMGetParam(func, 0);
  for (int i = 0; i < args_len; i++) {
    Ast *param_ast = ast->data.AST_LAMBDA.params + i;
    LLVMValueRef _param_val = codegen_tuple_access(
        i, param_val, symbol_data.llvm_params_obj_type, builder);
    match_values(param_ast, _param_val,
                 symbol_data.params_obj_type->data.T_CONS.args[i], &fn_ctx,
                 module, builder);
  }

  LLVMValueRef switch_val = LLVMGetParam(func, 1);
  _coroutine_ctx.switch_val = switch_val;

  LLVMValueRef switch_inst = LLVMBuildSwitch(
      builder, switch_val, _coroutine_ctx.block_refs[num_yields], num_yields);

  for (int i = 0; i < num_yields; i++) {
    LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt32Type(), i, 0),
                _coroutine_ctx.block_refs[i]);
  }

  // Build case 0
  LLVMPositionBuilderAtEnd(builder, _coroutine_ctx.block_refs[0]);

  LLVMValueRef body =
      codegen(ast->data.AST_LAMBDA.body, &fn_ctx, module, builder);

  LLVMValueRef str = LLVMGetUndef(symbol_data.llvm_ret_option_type);
  str = LLVMBuildInsertValue(builder, str, LLVMConstInt(LLVMInt8Type(), 1, 0),
                             0, "insert None tag");
  LLVMBuildRet(builder, str);

  _coroutine_ctx = prev_cr_ctx;

  LLVMPositionBuilderAtEnd(builder, prev_block);
  // LLVMDumpValue(func);
  return func;
}

/**
 * @brief Compile coroutine function and create special symbol for binding
 *
 * Coroutine has the form a -> b -> c -> (() -> Option of d)
 *
 * @details Example usage:
 * ```
 *
 * let f = fn a b c ->
 *   yield a;
 *   yield b;
 *   yield c;
 *   yield 4;
 *   yield 5
 *
 * ;;
 *
 * ```
 *
 * Has type Int -> Int -> Int -> (() -> Int)
 *
 * We call f here the coroutine 'generator'.
 * We can create an 'instance' of the coroutine like this:
 * ```
 *
 * let x = f 1 2 3;
 * x (); # Some 1
 *
 * x (); # Some 2
 *
 * x (); # Some 3
 *
 * x (); # Some 4
 *
 * x (); # Some 5
 *
 * x (); # None
 * ```
 *
 * @param ast The AST node representing the coroutine
 * @param ctx The JIT compiler context
 * @param module The LLVM module reference
 * @param builder The LLVM builder reference
 * @return LLVMValueRef The compiled coroutine value
 */
LLVMValueRef codegen_coroutine_binding(Ast *ast, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {

  Ast *def_ast = ast->data.AST_LET.expr;
  Type *fn_type = def_ast->md;
  size_t args_len = def_ast->data.AST_LAMBDA.len;

  coroutine_generator_symbol_data_t symbol_data = {
      .ast = def_ast,
      .stack_ptr = ctx->stack_ptr,
      .ret_option_type = empty_type(),
      .params_obj_type = empty_type(),
  };

  // Type *ret_option = empty_type();
  // Type *params_obj_type = empty_type();

  LLVMTypeRef llvm_params_obj_type =
      param_struct_type(fn_type, args_len, symbol_data.params_obj_type,
                        symbol_data.ret_option_type, ctx->env, module);

  LLVMTypeRef llvm_ret_option_type =
      type_to_llvm_type(symbol_data.ret_option_type, ctx->env, module);

  LLVMTypeRef def_fn_type = LLVMFunctionType(
      llvm_ret_option_type,
      (LLVMTypeRef[]){llvm_params_obj_type, LLVMInt32Type()}, 2, 0);

  symbol_data.llvm_ret_option_type = llvm_ret_option_type;
  symbol_data.llvm_params_obj_type = llvm_params_obj_type;
  symbol_data.def_fn_type = def_fn_type;

  LLVMValueRef def =
      codegen_coroutine_generator(def_ast, symbol_data, ctx, module, builder);

  JITSymbol *sym =
      new_symbol(STYPE_COROUTINE_GENERATOR, ast->md, def, LLVMTypeOf(def));
  sym->symbol_data.STYPE_COROUTINE_GENERATOR = symbol_data;

  Ast *binding = ast->data.AST_LET.binding;
  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
              hash_string(id_chars, id_len), sym);
  return def;
}

// given a compiled coroutine generator (@param symbol)
// create a struct containing the initial parameters, the function pointer
// returned by `codegen_coroutine_generator` and the switch index value 0
// (0, parameters, fn)
LLVMValueRef codegen_coroutine_instance(Ast *application,
                                        JITSymbol *generator_symbol,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {
  /* coroutine instance
   * let x = f 1 2 3 ...
   * x becomes a 'curried' version of the coroutine generator f
   * x = fn () ->
   *   fgen (1, 2, 3) 0
   * ;;
   *
   * at each call `x ()` - x must be replaced with
   * fn () ->
   *   fgen (a, b, c) (i+1)
   * ;;
   */

  LLVMTypeRef params_obj_type =
      generator_symbol->symbol_data.STYPE_COROUTINE_GENERATOR
          .llvm_params_obj_type;

  LLVMValueRef coroutine_def = generator_symbol->val;

  LLVMTypeRef instance_type = LLVMStructType(
      (LLVMTypeRef[]){
          LLVMInt32Type(),             // coroutine counter
          params_obj_type,             // params tuple
          generator_symbol->llvm_type, // coroutine generator function type
      },
      3, 0);

  LLVMValueRef instance = LLVMGetUndef(instance_type);
  LLVMValueRef params_obj =
      LLVMGetUndef(generator_symbol->symbol_data.STYPE_COROUTINE_GENERATOR
                       .llvm_params_obj_type);

  for (int i = 0; i < application->data.AST_APPLICATION.len; i++) {
    Ast *arg = application->data.AST_APPLICATION.args + i;
    params_obj = LLVMBuildInsertValue(
        builder, params_obj, codegen(arg, ctx, module, builder), i, "");
  }

  instance = LLVMBuildInsertValue(builder, instance,
                                  LLVMConstInt(LLVMInt32Type(), 0, 0), 0, "");
  instance = LLVMBuildInsertValue(builder, instance, params_obj, 1, "");
  instance =
      LLVMBuildInsertValue(builder, instance, generator_symbol->val, 2, "");

  return instance;
}

// given a symbol containing the coroutine:
// (parameters, fn, i)
// call fn(parameters,  i)
// and update the symbol to be (new_parameters, fn, i+1)
LLVMValueRef codegen_coroutine_next(Ast *application, JITSymbol *sym,
                                    Type *expected_fn_type, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  /* if x is the 'curried' version of the coroutine generator f at some point
   * during the coroutine
   * x = fn () ->
   *  fgen (1, 2, 3) i
   *  ;;
   *
   * at each call `x ()`, return the value from fgen (a, b, c) i but also x
   * must be replaced with
   * fn () ->
   *  fgen (a, b, c) (i+1)
   * ;;
   */
  LLVMValueRef instance = sym->val;
  LLVMTypeRef instance_type = LLVMTypeOf(instance);

  LLVMValueRef counter =
      codegen_tuple_access(0, instance, instance_type, builder);
  LLVMValueRef params =
      codegen_tuple_access(1, instance, instance_type, builder);
  LLVMValueRef func = codegen_tuple_access(2, instance, instance_type, builder);

  LLVMValueRef result = LLVMBuildCall2(
      builder, sym->symbol_data.STYPE_COROUTINE_INSTANCE.def_fn_type, func,
      (LLVMValueRef[]){params, counter}, 2, "coroutine_next");

  // increment counter
  counter =
      LLVMBuildAdd(builder, counter, LLVMConstInt(LLVMInt32Type(), 1, 0), "++");

  instance = LLVMGetUndef(instance_type);

  instance = LLVMBuildInsertValue(builder, instance, counter, 0, "");
  instance = LLVMBuildInsertValue(builder, instance, params, 1, "");
  instance = LLVMBuildInsertValue(builder, instance, func, 2, "");
  sym->val = instance;

  return result;
}