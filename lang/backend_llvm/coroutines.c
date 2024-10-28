#include "coroutines.h"
#include "coroutine_types.h"
#include "match.h"
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
  if (fn_len == 0 || fn_type->data.T_FN.from->kind == T_VOID) {
    param_obj_type->kind = T_VOID;
    return LLVMVoidType();
  }

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
  LLVMTypeRef instance_type;
  LLVMValueRef func;
  LLVMTypeRef func_type;
  int num_branches;
  int current_branch;
} coroutine_ctx_t;

void increment_instance_counter(LLVMValueRef instance_ptr,
                                LLVMTypeRef instance_type,
                                LLVMBuilderRef builder) {
  LLVMValueRef counter_gep = LLVMBuildStructGEP2(
      builder, instance_type, instance_ptr, 1, "instance_counter_ptr");
  LLVMValueRef counter =
      LLVMBuildLoad2(builder, LLVMInt32Type(), counter_gep, "instance_counter");
  counter = LLVMBuildAdd(builder, counter, LLVMConstInt(LLVMInt32Type(), 1, 0),
                         "instance_counter++");
  LLVMBuildStore(builder, counter, counter_gep);
}

static coroutine_ctx_t _coroutine_ctx = {};

static void copy_instance(LLVMValueRef dest_ptr, LLVMValueRef src_ptr,
                          LLVMTypeRef instance_type, LLVMBuilderRef builder) {

  LLVMValueRef size = LLVMSizeOf(instance_type);
  LLVMBuildMemCpy(builder, dest_ptr, 0, src_ptr, 0, size);

  // // Option 2: Copy field by field
  // unsigned field_count = LLVMCountStructElementTypes(instance_type);
  // for (unsigned i = 0; i < field_count; i++) {
  //   // Get pointer to field in source
  //   LLVMValueRef src_field_ptr = LLVMBuildStructGEP2(
  //       builder, instance_type, src_ptr, i, "src_field_ptr");
  //
  //   // Get pointer to field in destination
  //   LLVMValueRef dest_field_ptr = LLVMBuildStructGEP2(
  //       builder, instance_type, dest_ptr, i, "dest_field_ptr");
  //
  //   // Load from source
  //   LLVMValueRef field_val =
  //       LLVMBuildLoad2(builder, LLVMStructGetTypeAtIndex(instance_type, i),
  //                      src_field_ptr, "field_load");
  //
  //   // Store to destination
  //   LLVMBuildStore(builder, field_val, dest_field_ptr);
  // }
}
LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {

  LLVMValueRef func = _coroutine_ctx.func;
  LLVMValueRef instance_ptr = LLVMGetParam(func, 0);
  LLVMTypeRef instance_type = _coroutine_ctx.instance_type;

  Ast *expr = ast->data.AST_YIELD.expr;
  if (expr->tag == AST_APPLICATION) {
    JITSymbol *sym = lookup_id_ast(expr->data.AST_APPLICATION.function, ctx);
    if (sym->type == STYPE_COROUTINE_GENERATOR) {

      LLVMTypeRef instance_type = coroutine_instance_type(
          sym->symbol_data.STYPE_COROUTINE_GENERATOR.llvm_params_obj_type);

      LLVMValueRef new_instance_ptr = codegen_coroutine_instance(
          expr->data.AST_APPLICATION.args, expr->data.AST_APPLICATION.len, sym,
          ctx, module, builder);
      copy_instance(instance_ptr, new_instance_ptr, instance_type, builder);

      _coroutine_ctx.current_branch++;

      LLVMValueRef ret_opt = codegen_coroutine_next(expr, instance_ptr, instance_type,
                                    _coroutine_ctx.func_type, ctx, module,
                                    builder);
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

  _coroutine_ctx.current_branch++;
  increment_instance_counter(instance_ptr, instance_type, builder);

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

  ht *scope = fn_ctx->stack + fn_ctx->stack_ptr;
  ht_set_hash(scope, fn_name.chars, fn_name.hash, sym);
}

LLVMValueRef
codegen_coroutine_generator(Ast *ast,
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
  return func;
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

  LLVMValueRef def = codegen_coroutine_generator(
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

  LLVMValueRef instance = (ctx->stack_ptr == 0)
                              ? LLVMBuildMalloc(builder, instance_type, "")
                              : LLVMBuildAlloca(builder, instance_type, "");

  LLVMValueRef instance_gep = LLVMBuildStructGEP2(
      builder, instance_type, instance, 0, "instance_fn_ptr");
  LLVMBuildStore(builder, generator_symbol->val, instance_gep);

  instance_gep = LLVMBuildStructGEP2(builder, instance_type, instance, 1,
                                     "instance_counter_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), instance_gep);

  if (generator_symbol->symbol_data.STYPE_COROUTINE_GENERATOR.params_obj_type
          ->kind != T_VOID) {
    instance_gep = LLVMBuildStructGEP2(builder, instance_type, instance, 2,
                                       "instance_params_ptr");

    LLVMValueRef params_obj =
        LLVMGetUndef(generator_symbol->symbol_data.STYPE_COROUTINE_GENERATOR
                         .llvm_params_obj_type);

    for (int i = 0; i < args_len; i++) {
      Ast *arg = args + i;
      params_obj = LLVMBuildInsertValue(
          builder, params_obj, codegen(arg, ctx, module, builder), i, "");
    }
    LLVMBuildStore(builder, params_obj, instance_gep);
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
