#include "coroutines.h"
#include "coroutine_instance.h"
#include "function.h"
#include "list.h"
#include "match.h"
#include "serde.h"
#include "symbols.h"
#include "tuple.h"
#include "types.h"
#include "util.h"
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
  if (fn_len == 0 || fn_type->data.T_FN.from->kind == T_VOID) {
    param_obj_type->kind = T_VOID;
    return LLVMVoidType();
  }
  Type param_tuple = {T_CONS,
                      {.T_CONS = {.name = TYPE_NAME_TUPLE,
                                  .args = contained,
                                  .num_args = fn_len}}};

  *param_obj_type = param_tuple;

  return LLVMStructType(llvm_param_types, fn_len, 0);
}

typedef struct coroutine_ctx_t {
  LLVMValueRef switch_val;
  LLVMBasicBlockRef *block_refs;
  LLVMTypeRef ret_option_type;
  LLVMTypeRef instance_type;
  LLVMValueRef func;
  LLVMTypeRef func_type;
  int num_branches;
  int current_branch;
} coroutine_ctx_t;

static coroutine_ctx_t _coroutine_ctx = {};

static void copy_instance(LLVMValueRef dest_ptr, LLVMValueRef src_ptr,
                          LLVMTypeRef instance_type, LLVMBuilderRef builder) {
  LLVMValueRef size = LLVMSizeOf(instance_type);
  LLVMBuildMemCpy(builder, dest_ptr, 0, src_ptr, 0, size);
}

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMValueRef func = _coroutine_ctx.func;
  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMTypeRef instance_type = _coroutine_ctx.instance_type;
  increment_instance_counter(instance_ptr, instance_type, builder);
  _coroutine_ctx.current_branch++;

  Ast *expr = ast->data.AST_YIELD.expr;
  if (expr->tag == AST_APPLICATION) {
    JITSymbol *sym = lookup_id_ast(expr->data.AST_APPLICATION.function, ctx);
    if (sym->type == STYPE_COROUTINE_GENERATOR) {

      LLVMTypeRef instance_type = coroutine_instance_type(
          sym->symbol_data.STYPE_COROUTINE_GENERATOR.llvm_params_obj_type);
      bool is_same_recursive_ref =
          sym->symbol_data.STYPE_COROUTINE_GENERATOR.recursive_ref;
      if (is_same_recursive_ref) {
        // TODO: reuse already-allocated instance and replace that if recursive
        // ref
      }

      LLVMValueRef old_instance_ptr =
          replace_instance(heap_alloc(instance_type, ctx, builder),
                           instance_type, instance_ptr, builder);

      instance_ptr = replace_instance(
          instance_ptr, instance_type,
          codegen_coroutine_instance(expr->data.AST_APPLICATION.args,
                                     expr->data.AST_APPLICATION.len, sym, ctx,
                                     module, builder),
          builder);

      LLVMValueRef parent_gep =
          coroutine_instance_parent_gep(instance_ptr, instance_type, builder);
      LLVMBuildStore(builder, old_instance_ptr, parent_gep);

      LLVMValueRef ret_opt = codegen_coroutine_next(
          expr, instance_ptr, instance_type, _coroutine_ctx.func_type, ctx,
          module, builder);
      LLVMBuildRet(builder, ret_opt);
      LLVMPositionBuilderAtEnd(
          builder, _coroutine_ctx.block_refs[_coroutine_ctx.current_branch]);
      return ret_opt;
    }
  }

  LLVMValueRef val = codegen(expr, ctx, module, builder);
  LLVMValueRef ret_opt = LLVMGetUndef(_coroutine_ctx.ret_option_type);

  ret_opt =
      LLVMBuildInsertValue(builder, ret_opt, LLVMConstInt(LLVMInt8Type(), 0, 0),
                           0, "insert Some tag");

  ret_opt = LLVMBuildInsertValue(builder, ret_opt, val, 1, "insert Some Value");

  LLVMBuildRet(builder, ret_opt);
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
  sym->symbol_data.STYPE_COROUTINE_GENERATOR.recursive_ref = true;

  ht *scope = fn_ctx->stack + fn_ctx->stack_ptr;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
}

LLVMValueRef coroutine_default_block(LLVMValueRef instance,
                                     LLVMTypeRef instance_type,
                                     LLVMTypeRef ret_opt_type,
                                     LLVMBuilderRef builder) {

  // Create basic block for the non-null path
  LLVMBasicBlockRef non_null_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "non_null_path");
  LLVMBasicBlockRef continue_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "continue");

  LLVMValueRef parent_gep =
      coroutine_instance_parent_gep(instance, instance_type, builder);

  // Load the parent pointer value
  LLVMValueRef parent_ptr = LLVMBuildLoad2(
      builder, LLVMPointerType(LLVMInt8Type(), 0), parent_gep, "parent_ptr");

  // Create the null comparison
  LLVMValueRef is_null = LLVMBuildICmp(
      builder, LLVMIntEQ, parent_ptr,
      LLVMConstNull(LLVMPointerType(LLVMInt8Type(), 0)), "is_null");

  // Create the conditional branch
  LLVMBuildCondBr(builder, is_null, continue_block, non_null_block);

  // Non-null path - Parent coroutine exists
  LLVMPositionBuilderAtEnd(builder, non_null_block);
  // Cast the parent pointer to the correct instance type
  LLVMValueRef parent_instance = LLVMBuildPointerCast(
      builder, parent_ptr, LLVMPointerType(instance_type, 0),
      "parent_instance");

  instance =
      replace_instance(instance, instance_type, parent_instance, builder);
  increment_instance_counter(instance, instance_type, builder);

  LLVMValueRef func = codegen_tuple_access(0, instance, instance_type, builder);

  LLVMValueRef ret_opt = LLVMBuildCall2(
      builder, coroutine_def_fn_type(instance_type, ret_opt_type), func,
      (LLVMValueRef[]){parent_instance}, 1, "coroutine_next");

  LLVMBuildRet(builder, ret_opt);

  // Continue with original null case
  LLVMPositionBuilderAtEnd(builder, continue_block);
  // Original null case code
  LLVMValueRef str = LLVMGetUndef(ret_opt_type);
  str = LLVMBuildInsertValue(builder, str, LLVMConstInt(LLVMInt8Type(), 1, 0),
                             0, "insert None tag");
  LLVMBuildRet(builder, str);
}

LLVMValueRef
compile_coroutine_generator(Ast *ast,
                            coroutine_generator_symbol_data_t symbol_data,
                            LLVMTypeRef instance_type, JITLangCtx *ctx,
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
  _coroutine_ctx.func = func;
  _coroutine_ctx.func_type = symbol_data.def_fn_type;
  _coroutine_ctx.instance_type = instance_type;

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

  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  if ((args_len == 1 && fn_type->data.T_FN.from->kind == T_VOID) ||
      (args_len == 0)) {
    // printf("don't need params tuple\n");
  } else {
    LLVMValueRef params_tuple =
        codegen_tuple_access(2, instance_ptr, instance_type, builder);

    for (size_t i = 0; i < args_len; i++) {
      Ast *param_ast = ast->data.AST_LAMBDA.params + i;

      LLVMValueRef _param_val = codegen_tuple_access(
          i, instance_ptr, symbol_data.llvm_params_obj_type, builder);

      match_values(param_ast, _param_val,
                   symbol_data.params_obj_type->data.T_CONS.args[i], &fn_ctx,
                   module, builder);
    }
  }

  LLVMValueRef switch_val =
      codegen_tuple_access(1, instance_ptr, instance_type, builder);
  _coroutine_ctx.switch_val = switch_val;

  LLVMValueRef switch_inst = LLVMBuildSwitch(
      builder, switch_val, _coroutine_ctx.block_refs[num_yields], num_yields);

  for (int i = 0; i < num_yields; i++) {
    LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt32Type(), i, 0),
                _coroutine_ctx.block_refs[i]);
  }

  LLVMPositionBuilderAtEnd(builder, _coroutine_ctx.block_refs[0]);

  LLVMValueRef body =
      codegen(ast->data.AST_LAMBDA.body, &fn_ctx, module, builder);

  coroutine_default_block(instance_ptr, instance_type,
                          symbol_data.llvm_ret_option_type, builder);
  _coroutine_ctx = prev_cr_ctx;
  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}

LLVMValueRef codegen_generic_coroutine_binding(Ast *ast, JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder) {
  Ast *binding = ast->data.AST_LET.binding;
  Ast *def_ast = ast->data.AST_LET.expr;
  Type *fn_type = def_ast->md;

  JITSymbol *sym =
      new_symbol(STYPE_GENERIC_COROUTINE_GENERATOR, fn_type, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_COROUTINE_GENERATOR.ast = def_ast;
  sym->symbol_data.STYPE_GENERIC_COROUTINE_GENERATOR.stack_ptr = ctx->stack_ptr;

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
              hash_string(id_chars, id_len), sym);
  return NULL;
}
LLVMValueRef codegen_specific_coroutine(JITSymbol *sym, const char *sym_name,
                                        Type *expected_type, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  printf("create specific coroutine instance %s\n", sym_name);
  print_type(expected_type);
  print_type(sym->symbol_data.STYPE_GENERIC_COROUTINE_GENERATOR.ast->md);

  SpecificFns *specific_fns =
      sym->symbol_data.STYPE_GENERIC_FUNCTION.specific_fns;
  LLVMValueRef callable = specific_fns_lookup(specific_fns, expected_type);

  if (callable) {
    printf("found compiled coroutine def\n");
  } else {
    printf("compile new specific coroutine def\n");
  }

  return NULL;
}

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

  // get correct param obj type
  LLVMTypeRef llvm_params_obj_type =
      param_struct_type(fn_type, args_len, symbol_data.params_obj_type,
                        symbol_data.ret_option_type, ctx->env, module);
  symbol_data.llvm_params_obj_type = llvm_params_obj_type;

  // get correct return option type
  LLVMTypeRef llvm_ret_option_type =
      type_to_llvm_type(symbol_data.ret_option_type, ctx->env, module);
  symbol_data.llvm_ret_option_type = llvm_ret_option_type;

  LLVMTypeRef instance_type = coroutine_instance_type(llvm_params_obj_type);

  LLVMTypeRef def_fn_type =
      coroutine_def_fn_type(instance_type, llvm_ret_option_type);
  symbol_data.def_fn_type = def_fn_type;

  LLVMValueRef def = compile_coroutine_generator(
      def_ast, symbol_data, instance_type, ctx, module, builder);

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
LLVMValueRef codegen_coroutine_instance(Ast *args, int args_len,
                                        JITSymbol *generator_symbol,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  LLVMTypeRef params_obj_type =
      generator_symbol->symbol_data.STYPE_COROUTINE_GENERATOR
          .llvm_params_obj_type;

  LLVMValueRef coroutine_def = generator_symbol->val;

  LLVMTypeRef instance_type = coroutine_instance_type(params_obj_type);

  LLVMValueRef instance = heap_alloc(instance_type, ctx, builder);

  LLVMValueRef fn_gep =
      coroutine_instance_fn_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, generator_symbol->val, fn_gep);

  LLVMValueRef counter_gep =
      coroutine_instance_counter_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter_gep);

  LLVMValueRef parent_gep =
      coroutine_instance_parent_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, LLVMConstNull(LLVMPointerType(LLVMInt8Type(), 0)),
                 parent_gep);

  LLVMValueRef params_gep =
      coroutine_instance_params_gep(instance, instance_type, builder);

  if (params_gep) {
    LLVMValueRef params_obj =
        LLVMGetUndef(generator_symbol->symbol_data.STYPE_COROUTINE_GENERATOR
                         .llvm_params_obj_type);

    for (int i = 0; i < args_len; i++) {
      Ast *arg = args + i;
      params_obj = LLVMBuildInsertValue(
          builder, params_obj, codegen(arg, ctx, module, builder), i, "");
    }
    LLVMBuildStore(builder, params_obj, params_gep);
  }

  return instance;
}

// given a symbol containing the coroutine:
// (parameters, fn, i)
// call fn(parameters,  i)
// and update the symbol to be (new_parameters, fn, i+1)
LLVMValueRef codegen_coroutine_next(Ast *application, LLVMValueRef instance,
                                    LLVMTypeRef instance_type,
                                    LLVMTypeRef def_fn_type, JITLangCtx *ctx,
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
  LLVMValueRef func = codegen_tuple_access(0, instance, instance_type, builder);
  LLVMValueRef result =
      LLVMBuildCall2(builder, def_fn_type, func, (LLVMValueRef[]){instance}, 1,
                     "coroutine_next");

  return result;
}

LLVMValueRef list_iter_instance(Ast *ast, LLVMValueRef func, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  Ast *list_ast = ast->data.AST_APPLICATION.args;
  LLVMValueRef list = codegen(list_ast, ctx, module, builder);
  Type *ret_opt_type = ast->md;
  ret_opt_type = ret_opt_type->data.T_FN.to;
  Type *list_el_type = type_of_option(ret_opt_type);

  LLVMTypeRef instance_type =
      coroutine_instance_type(list_type(list_el_type, ctx->env, module));

  return LLVMConstInt(LLVMInt32Type(), 1, 0);
}
LLVMValueRef coroutine_list_iter_generator_fn(Type *expected_type,
                                              JITLangCtx *ctx,
                                              LLVMModuleRef module,
                                              LLVMBuilderRef builder) {
  Type *ret_opt_type = expected_type;
  ret_opt_type = ret_opt_type->data.T_FN.to;
  Type *list_el_type = type_of_option(ret_opt_type);
  LLVMTypeRef llvm_list_el_type =
      type_to_llvm_type(list_el_type, ctx->env, module);
  LLVMTypeRef llvm_list_type = list_type(list_el_type, ctx->env, module);
  LLVMTypeRef instance_type = coroutine_instance_type(llvm_list_type);
  LLVMTypeRef llvm_ret_opt_type =
      type_to_llvm_type(ret_opt_type, ctx->env, module);

  LLVMTypeRef func_type =
      coroutine_def_fn_type(instance_type, llvm_ret_opt_type);

  LLVMValueRef func = LLVMAddFunction(module, "list_iter_generator", func_type);

  LLVMSetLinkage(func, LLVMExternalLinkage);

  JITLangCtx fn_ctx = ctx_push(*ctx);
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef instance = LLVMGetParam(func, 0);
  LLVMValueRef list_head =
      codegen_tuple_access(2, instance, instance_type, builder);

  // Create basic block for the non-null path
  LLVMBasicBlockRef non_null_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "non_null_path");
  LLVMBasicBlockRef continue_block = LLVMAppendBasicBlock(
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "continue");

  LLVMValueRef parent_gep =
      coroutine_instance_parent_gep(instance, instance_type, builder);

  // Load the parent pointer value
  LLVMValueRef parent_ptr = LLVMBuildLoad2(
      builder, LLVMPointerType(LLVMInt8Type(), 0), parent_gep, "parent_ptr");

  // Create the null comparison
  LLVMValueRef is_null = ll_is_null(list_head, llvm_list_el_type, builder);

  // Create the conditional branch
  LLVMBuildCondBr(builder, is_null, continue_block, non_null_block);

  // Non-null path - list continues
  LLVMPositionBuilderAtEnd(builder, non_null_block);
  LLVMValueRef some = LLVMGetUndef(llvm_ret_opt_type);
  some = LLVMBuildInsertValue(builder, some, LLVMConstInt(LLVMInt8Type(), 0, 0),
                              0, "insert Some tag");
  some = LLVMBuildInsertValue(
      builder, some, ll_get_head_val(list_head, llvm_list_el_type, builder), 1,
      "insert Some val");

  // replace instance with next list element and return head value
  LLVMValueRef list_next = ll_get_next(list_head, llvm_list_el_type, builder);
  LLVMValueRef param_gep =
      coroutine_instance_params_gep(instance, instance_type, builder);
  LLVMBuildStore(builder, list_next, param_gep);
  increment_instance_counter(instance, instance_type, builder);

  LLVMBuildRet(builder, some);

  // Continue with original null case
  LLVMPositionBuilderAtEnd(builder, continue_block);

  increment_instance_counter(instance, instance_type, builder);
  // update instance with parent if exists
  // Original null case code
  LLVMValueRef none = LLVMGetUndef(llvm_ret_opt_type);
  none = LLVMBuildInsertValue(builder, none, LLVMConstInt(LLVMInt8Type(), 1, 0),
                              0, "insert None tag");
  LLVMBuildRet(builder, none);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  // LLVMDumpValue(func);
  return func;
}

LLVMValueRef array_iter_instance(Ast *ast, LLVMValueRef func, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  return LLVMConstInt(LLVMInt32Type(), 1, 0);
}

LLVMValueRef coroutine_array_iter_generator_fn(Type *expected_type) {}
