#include "./dsp_build_expr.hpp"
#include "./compile_synth_mlir.hpp"

extern "C" {
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/ht.h"
#include "../../lang/parse.h"
#include "../../lang/serde.h"
#include "../../lang/types/type.h"
#include "../../lang/types/type_ser.h"
#include "./lib.h"
}

#include "./dsp_dialect.hpp"
#include <mlir/Dialect/Arith/IR/Arith.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr int kSinTabSize = 1 << 11;
constexpr int kSqTabSize = 1 << 11;
constexpr int kSawTabSize = 1 << 11;

} // namespace

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static mlir::Value coerce_to_f64(mlir::Value v, mlir::OpBuilder &b,
                                 mlir::Location loc) {
  auto ty = v.getType();

  if (ty.isF64())
    return v;
  if (ty.isInteger(1) || ty.isInteger(32) || ty.isInteger(64))
    return b.create<mlir::arith::SIToFPOp>(loc, b.getF64Type(), v);
  if (ty.isF32())
    return b.create<mlir::arith::ExtFOp>(loc, b.getF64Type(), v);

  return v;
}

static mlir::Value coerce_to_i32(mlir::Value v, mlir::OpBuilder &b,
                                 mlir::Location loc) {
  auto ty = v.getType();

  if (ty.isInteger(32))
    return v;

  if (ty.isInteger(1))
    return b.create<mlir::arith::ExtSIOp>(loc, b.getI32Type(), v);

  if (ty.isInteger(64))
    return b.create<mlir::arith::TruncIOp>(loc, b.getI32Type(), v);

  if (ty.isF64() || ty.isF32())
    return b.create<mlir::arith::FPToSIOp>(loc, b.getI32Type(), v);

  return v;
}

static mlir::Type mlir_type_from_ylc_type(Type *type, mlir::OpBuilder &b) {

  if (!type) {
    return {};
  }

  print_type(type);

  switch (type->kind) {
  case T_NUM:
    return b.getF64Type();
  case T_INT:
    return b.getI32Type();
  case T_BOOL:
    return b.getI1Type();
  case T_VOID:
    return {};
  case T_CONS: {
    if (is_array_type(type) || is_list_type(type)) {
      return mlir_type_from_ylc_type(type->data.T_CONS.args[0], b);
      // b.getA
    }
    return {};
  }
  case T_VAR:
  case T_TYPECLASS_RESOLVE: {
    // TODO: this is a hack - needs more nuanced handling
    return b.getF64Type();
  }

  default:

    return {};
  }
}

static mlir::Type mlir_array_elem_type_from_ylc(Type *type,
                                                mlir::OpBuilder &b) {
  print_type(type);
  if (is_array_type(type) || is_list_type(type)) {
    return mlir_type_from_ylc_type(type->data.T_CONS.args[0], b);
  }
  return {};
}

static mlir::Value coerce_to_type(mlir::Value v, mlir::Type ty,
                                  mlir::OpBuilder &b, mlir::Location loc) {
  if (!v || !ty)
    return {};
  if (v.getType() == ty)
    return v;

  if (ty.isF64())
    return coerce_to_f64(v, b, loc);

  if (ty.isInteger(32)) {
    if (v.getType().isF64())
      return b.create<mlir::arith::FPToSIOp>(loc, ty, v);
    if (v.getType().isInteger(1) || v.getType().isInteger(64))
      return b.create<mlir::arith::TruncIOp>(loc, ty, v);
  }

  if (ty.isInteger(1)) {
    if (v.getType().isInteger(32) || v.getType().isInteger(64))
      return b.create<mlir::arith::TruncIOp>(loc, ty, v);
  }

  return v;
}

static bool is_float_like(mlir::Type ty) {
  return ty && llvm::isa<mlir::FloatType>(ty);
}

static bool is_int_like(mlir::Type ty) {
  return ty && llvm::isa<mlir::IntegerType>(ty);
}

static mlir::Type select_numeric_binary_type(mlir::Value lhs, mlir::Value rhs,
                                             mlir::OpBuilder &b) {
  mlir::Type lhs_ty = lhs.getType();
  mlir::Type rhs_ty = rhs.getType();

  if (lhs_ty.isF64() || rhs_ty.isF64())
    return b.getF64Type();
  if (lhs_ty.isF32() || rhs_ty.isF32())
    return b.getF32Type();

  if (lhs_ty.isInteger(32) || rhs_ty.isInteger(32))
    return b.getI32Type();
  if (lhs_ty.isInteger(64) || rhs_ty.isInteger(64))
    return b.getI64Type();
  if (lhs_ty.isInteger(1) || rhs_ty.isInteger(1))
    return b.getI1Type();

  return {};
}

static bool coerce_binary_numeric_operands(mlir::Value &lhs, mlir::Value &rhs,
                                           mlir::OpBuilder &b,
                                           mlir::Location loc) {
  mlir::Type target_ty = select_numeric_binary_type(lhs, rhs, b);
  if (!target_ty)
    return false;

  lhs = coerce_to_type(lhs, target_ty, b, loc);
  rhs = coerce_to_type(rhs, target_ty, b, loc);
  return lhs && rhs && lhs.getType() == target_ty && rhs.getType() == target_ty;
}

static mlir::func::FuncOp
get_or_create_extern_decl(mlir::ModuleOp module, mlir::OpBuilder &b,
                          mlir::Location loc, Ast *extern_ast, Type *fn_type) {
  if (!extern_ast || extern_ast->tag != AST_EXTERN_FN || !fn_type)
    return {};

  const char *name = extern_ast->data.AST_EXTERN_FN.fn_name.chars;
  if (!name)
    return {};

  if (auto fn = module.lookupSymbol<mlir::func::FuncOp>(name))
    return fn;

  llvm::SmallVector<mlir::Type> inputs;
  Type *cur = fn_type;
  while (cur && cur->kind == T_FN) {
    if (cur->data.T_FN.from && cur->data.T_FN.from->kind != T_VOID) {
      auto argTy = mlir_type_from_ylc_type(cur->data.T_FN.from, b);
      if (!argTy)
        return {};
      inputs.push_back(argTy);
    }
    cur = cur->data.T_FN.to;
  }

  llvm::SmallVector<mlir::Type> results;
  auto retTy = mlir_type_from_ylc_type(cur, b);
  if (cur && cur->kind != T_VOID) {
    if (!retTy)
      return {};
    results.push_back(retTy);
  }

  mlir::OpBuilder::InsertionGuard guard(b);
  b.setInsertionPointToStart(module.getBody());
  auto fn = b.create<mlir::func::FuncOp>(loc, name,
                                         b.getFunctionType(inputs, results));
  fn.setPrivate();
  return fn;
}

static mlir::func::FuncOp
get_or_create_runtime_decl(mlir::ModuleOp module, mlir::OpBuilder &b,
                           mlir::Location loc, llvm::StringRef name,
                           mlir::FunctionType fnType) {
  if (auto fn = module.lookupSymbol<mlir::func::FuncOp>(name))
    return fn;

  mlir::OpBuilder::InsertionGuard guard(b);
  b.setInsertionPointToStart(module.getBody());
  auto fn = b.create<mlir::func::FuncOp>(loc, name, fnType);
  fn.setPrivate();
  return fn;
}

static mlir::Value build_top_level_var_load(Ast *ast, JITSymbol *sym,
                                            DspBuildCtx &dsp_ctx) {
  auto &b = dsp_ctx.builder;
  auto loc = dsp_ctx.loc;
  auto module = dsp_ctx.frame_fn->getParentOfType<mlir::ModuleOp>();
  mlir::Type resultTy = mlir_type_from_ylc_type(ast->type, b);
  if (!resultTy)
    return {};

  llvm::StringRef fnName;
  if (resultTy.isF64()) {
    fnName = "ylc_mlir_get_global_f64";
  } else if (resultTy.isInteger(32)) {
    fnName = "ylc_mlir_get_global_i32";
  } else if (resultTy.isInteger(1)) {
    fnName = "ylc_mlir_get_global_i1";
  } else {
    return {};
  }

  auto callee = get_or_create_runtime_decl(
      module, b, loc, fnName, b.getFunctionType({b.getI32Type()}, {resultTy}));
  auto slot = b.create<mlir::arith::ConstantOp>(
      loc, b.getI32IntegerAttr(sym->symbol_data.STYPE_TOP_LEVEL_VAR));
  auto call = b.create<mlir::func::CallOp>(loc, callee,
                                           mlir::ValueRange{slot.getResult()});
  return call.getResult(0);
}

// Store a heap-allocated mlir::Value* in a JITSymbol's val field (which is
// LLVMValueRef, i.e. an opaque pointer).  We roundtrip through void* to avoid
// a C++ reinterpret_cast between unrelated pointer types.
static inline LLVMValueRef mlir_val_to_sym_val(mlir::Value *v) {
  return reinterpret_cast<LLVMValueRef>(static_cast<void *>(v));
}
static inline mlir::Value *sym_val_to_mlir_val(LLVMValueRef v) {
  return static_cast<mlir::Value *>(reinterpret_cast<void *>(v));
}

// ---------------------------------------------------------------------------
// Main walker
// ---------------------------------------------------------------------------

mlir::Value dsp_build_expr(Ast *ast, DspBuildCtx &dsp_ctx, JITLangCtx *ctx) {
  auto &b = dsp_ctx.builder;
  auto loc = dsp_ctx.loc;

  switch (ast->tag) {

  case AST_BODY: {
    mlir::Value val;
    AST_LIST_ITER(ast->data.AST_BODY.stmts,
                  ({ val = dsp_build_expr(l->ast, dsp_ctx, ctx); }));
    return val;
  }

  case AST_INT: {
    int v = ast->data.AST_INT.value;
    return b.create<mlir::arith::ConstantOp>(loc, b.getI32IntegerAttr(v));
  }

  case AST_FLOAT: {
    double v = (double)ast->data.AST_FLOAT.value;
    return b.create<mlir::arith::ConstantOp>(loc, b.getF64FloatAttr(v));
  }

  case AST_DOUBLE: {
    return b.create<mlir::arith::ConstantOp>(
        loc, b.getF64FloatAttr(ast->data.AST_DOUBLE.value));
  }

  case AST_BOOL: {
    return b.create<mlir::arith::ConstantOp>(
        loc, b.getIntegerAttr(b.getI1Type(), ast->data.AST_BOOL.value ? 1 : 0));
  }

  case AST_IDENTIFIER: {
    const char *name = ast->data.AST_IDENTIFIER.value;
    if (name) {
      if (strcmp(name, "sample_rate") == 0) {
        return b.create<mlir::arith::ConstantOp>(
            loc, b.getF64FloatAttr((double)dsp_ctx.sample_rate));
      }
      if (strcmp(name, "spf") == 0) {
        return b.create<mlir::arith::ConstantOp>(
            loc, b.getF64FloatAttr(dsp_ctx.spf));
      }
    }
    JITSymbol *sym = lookup_id_ast(ast, ctx);
    if (!sym) {
      fprintf(stderr, "dsp_build_expr: unresolved identifier '%s'\n",
              name ? name : "(null)");
      return {};
    }
    if (!sym->val) {
      if (sym->type == STYPE_TOP_LEVEL_VAR) {
        mlir::Value global = build_top_level_var_load(ast, sym, dsp_ctx);
        if (global)
          return global;
      }
      fprintf(stderr, "dsp_build_expr: identifier '%s' has null val\n",
              name ? name : "(null)");
      return {};
    }
    if (sym->type == STYPE_TOP_LEVEL_VAR) {
      mlir::Value global = build_top_level_var_load(ast, sym, dsp_ctx);
      if (global)
        return global;
      fprintf(stderr, "dsp_build_expr: identifier '%s' top-level load failed\n",
              name ? name : "(null)");
      return {};
    }
    if (sym->type != STYPE_LOCAL_VAR) {
      return {};
    }
    return *sym_val_to_mlir_val(sym->val);
  }

  case AST_RECORD_ACCESS: {
    JITSymbol *sym = lookup_id_ast(ast, ctx);
    if (!sym) {
      fprintf(stderr, "dsp_build_expr: unresolved record access\n");
      print_ast_err(ast);
      return {};
    }

    if (!sym->val) {
      if (sym->type == STYPE_TOP_LEVEL_VAR) {
        mlir::Value global = build_top_level_var_load(ast, sym, dsp_ctx);
        if (global)
          return global;
      }
      return {};
    }
    if (sym->type == STYPE_TOP_LEVEL_VAR) {
      mlir::Value global = build_top_level_var_load(ast, sym, dsp_ctx);
      if (global)
        return global;
      return {};
    }
    if (sym->type != STYPE_LOCAL_VAR) {
      return {};
    }
    return *sym_val_to_mlir_val(sym->val);
  }

  case AST_LET: {
    Ast *binding = ast->data.AST_LET.binding;
    Ast *expr = ast->data.AST_LET.expr;
    Ast *in_expr = ast->data.AST_LET.in_expr;

    JITLangCtx inner_ctx;
    JITLangCtx *work_ctx = ctx;
    bool pushed = false;
    if (in_expr) {
      STACK_ALLOC_CTX_PUSH(_inner, ctx)
      inner_ctx = _inner;
      work_ctx = &inner_ctx;
      pushed = true;
    }

    mlir::Value val = dsp_build_expr(expr, dsp_ctx, work_ctx);
    if (!val) {
      fprintf(stderr,
              "dsp_build_expr: could not build value for let binding\n");
      print_ast_err(ast);
      if (pushed)
        destroy_ctx(work_ctx);
      return {};
    }

    if (binding->tag == AST_TUPLE) {
      fprintf(stderr, "dsp_build_expr: tuple let-binding not yet supported\n");
      if (pushed)
        destroy_ctx(work_ctx);
      return {};
    }

    const char *chars = binding->data.AST_IDENTIFIER.value;
    int len = binding->data.AST_IDENTIFIER.length;
    auto *v = new mlir::Value(val);
    JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, expr->type,
                                mlir_val_to_sym_val(v), nullptr);
    ht_set_hash(work_ctx->frame->table, chars, hash_string(chars, (size_t)len),
                sym);

    if (in_expr) {
      mlir::Value e_val = dsp_build_expr(in_expr, dsp_ctx, work_ctx);
      if (pushed)
        destroy_ctx(work_ctx);
      return e_val;
    }

    return val;
  }

  case AST_APPLICATION: {
    Ast *f = ast->data.AST_APPLICATION.function;
    int nargs = is_void_func(f->type) ? 0 : ast->data.AST_APPLICATION.len;
    Ast *args = nargs > 0 ? ast->data.AST_APPLICATION.args : nullptr;

    // if (f->tag != AST_IDENTIFIER) {
    //   return {};
    // }
    //

    if (f->tag == AST_IDENTIFIER) {
      const char *fn = f->data.AST_IDENTIFIER.value;

      if (strcmp(fn, "phasor_sinc") == 0) {
        mlir::Value freq = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value trig = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!freq || !trig)
          return {};
        freq = coerce_to_f64(freq, b, loc);
        trig = coerce_to_f64(trig, b, loc);
        return b.create<dsp::PhasorSyncOp>(loc, freq, trig, dsp_ctx.spf)
            ->getResult(0);
      }
      if (strcmp(fn, "phasor") == 0) {
        mlir::Value freq = dsp_build_expr(args, dsp_ctx, ctx);
        if (!freq)
          return {};
        freq = coerce_to_f64(freq, b, loc);
        return b.create<dsp::PhasorOp>(loc, freq, dsp_ctx.spf)->getResult(0);
      }

      if (strcmp(fn, "sin_osc") == 0) {
        mlir::Value freq = dsp_build_expr(args, dsp_ctx, ctx);
        freq = coerce_to_f64(freq, b, loc);
        auto phasor =
            b.create<dsp::PhasorOp>(loc, freq, dsp_ctx.spf)->getResult(0);
        return b
            .create<dsp::Tabread2Op>(loc, phasor, "ylc_sin_table", kSinTabSize)
            ->getResult(0);
      }
      if (strcmp(fn, "sq_osc") == 0) {
        mlir::Value freq = dsp_build_expr(args, dsp_ctx, ctx);
        freq = coerce_to_f64(freq, b, loc);
        auto phasor =
            b.create<dsp::PhasorOp>(loc, freq, dsp_ctx.spf)->getResult(0);
        return b
            .create<dsp::Tabread2Op>(loc, phasor, "ylc_sq_table", kSqTabSize)
            ->getResult(0);
      }
      if (strcmp(fn, "saw_osc") == 0) {
        mlir::Value freq = dsp_build_expr(args, dsp_ctx, ctx);
        freq = coerce_to_f64(freq, b, loc);
        auto phasor =
            b.create<dsp::PhasorOp>(loc, freq, dsp_ctx.spf)->getResult(0);
        return b
            .create<dsp::Tabread2Op>(loc, phasor, "ylc_saw_table", kSawTabSize)
            ->getResult(0);
      }

      if (strcmp(fn, "+") == 0) {
        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};
        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};
        if (is_float_like(lhs.getType()))
          return b.create<mlir::arith::AddFOp>(loc, lhs, rhs);
        return b.create<mlir::arith::AddIOp>(loc, lhs, rhs);
      }
      if (strcmp(fn, "-") == 0) {

        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};
        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};
        if (is_float_like(lhs.getType()))
          return b.create<mlir::arith::SubFOp>(loc, lhs, rhs);
        return b.create<mlir::arith::SubIOp>(loc, lhs, rhs);
      }
      if (strcmp(fn, "*") == 0) {
        print_ast(ast);

        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};
        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};
        if (is_float_like(lhs.getType()))
          return b.create<mlir::arith::MulFOp>(loc, lhs, rhs);
        return b.create<mlir::arith::MulIOp>(loc, lhs, rhs);
      }
      if (strcmp(fn, "/") == 0) {

        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};
        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};
        if (is_float_like(lhs.getType()))
          return b.create<mlir::arith::DivFOp>(loc, lhs, rhs);
        return b.create<mlir::arith::DivSIOp>(loc, lhs, rhs);
      }
      if (strcmp(fn, "%") == 0) {

        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};
        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};
        if (is_float_like(lhs.getType()))
          return b.create<mlir::arith::RemFOp>(loc, lhs, rhs);
        return b.create<mlir::arith::RemSIOp>(loc, lhs, rhs);
      }
      if (strcmp(fn, "<") == 0) {

        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};
        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};
        if (is_float_like(lhs.getType()))
          return b.create<mlir::arith::CmpFOp>(
              loc, mlir::arith::CmpFPredicate::OLT, lhs, rhs);
        return b.create<mlir::arith::CmpIOp>(
            loc, mlir::arith::CmpIPredicate::slt, lhs, rhs);
      }
      if (strcmp(fn, "<=") == 0) {

        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};
        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};
        if (is_float_like(lhs.getType()))
          return b.create<mlir::arith::CmpFOp>(
              loc, mlir::arith::CmpFPredicate::OLE, lhs, rhs);
        return b.create<mlir::arith::CmpIOp>(
            loc, mlir::arith::CmpIPredicate::sle, lhs, rhs);
      }
      if (strcmp(fn, ">") == 0) {

        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};
        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};
        if (is_float_like(lhs.getType()))
          return b.create<mlir::arith::CmpFOp>(
              loc, mlir::arith::CmpFPredicate::OGT, lhs, rhs);
        return b.create<mlir::arith::CmpIOp>(
            loc, mlir::arith::CmpIPredicate::sgt, lhs, rhs);
      }
      if (strcmp(fn, ">=") == 0) {

        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};
        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};
        if (is_float_like(lhs.getType()))
          return b.create<mlir::arith::CmpFOp>(
              loc, mlir::arith::CmpFPredicate::OGE, lhs, rhs);
        return b.create<mlir::arith::CmpIOp>(
            loc, mlir::arith::CmpIPredicate::sge, lhs, rhs);
      }
      if (strcmp(fn, "==") == 0) {

        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);

        if (!lhs || !rhs)
          return {};

        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};

        if (is_float_like(lhs.getType()))
          return b.create<mlir::arith::CmpFOp>(
              loc, mlir::arith::CmpFPredicate::OEQ, lhs, rhs);

        return b.create<mlir::arith::CmpIOp>(
            loc, mlir::arith::CmpIPredicate::eq, lhs, rhs);
      }
      if (strcmp(fn, "!=") == 0) {

        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};
        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};
        if (is_float_like(lhs.getType()))
          return b.create<mlir::arith::CmpFOp>(
              loc, mlir::arith::CmpFPredicate::ONE, lhs, rhs);
        return b.create<mlir::arith::CmpIOp>(
            loc, mlir::arith::CmpIPredicate::ne, lhs, rhs);
      }

      if (strcmp(fn, "||") == 0) {
        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};

        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};

        if (lhs.getType().isInteger(1))
          return b.create<mlir::arith::OrIOp>(loc, lhs, rhs);

        if (is_float_like(lhs.getType())) {
          auto zero = b.create<mlir::arith::ConstantOp>(
              loc, mlir::FloatAttr::get(lhs.getType(), 0.0));
          auto one = b.create<mlir::arith::ConstantOp>(
              loc, mlir::FloatAttr::get(lhs.getType(), 1.0));

          auto lhs_nz = b.create<mlir::arith::CmpFOp>(
              loc, mlir::arith::CmpFPredicate::ONE, lhs, zero);
          auto rhs_nz = b.create<mlir::arith::CmpFOp>(
              loc, mlir::arith::CmpFPredicate::ONE, rhs, zero);
          auto any = b.create<mlir::arith::OrIOp>(loc, lhs_nz, rhs_nz);
          return b.create<mlir::arith::SelectOp>(loc, any, one, zero);
        }

        auto zero = b.create<mlir::arith::ConstantOp>(
            loc, b.getIntegerAttr(lhs.getType(), 0));
        auto one = b.create<mlir::arith::ConstantOp>(
            loc, b.getIntegerAttr(lhs.getType(), 1));

        auto lhs_nz = b.create<mlir::arith::CmpIOp>(
            loc, mlir::arith::CmpIPredicate::ne, lhs, zero);
        auto rhs_nz = b.create<mlir::arith::CmpIOp>(
            loc, mlir::arith::CmpIPredicate::ne, rhs, zero);
        auto any = b.create<mlir::arith::OrIOp>(loc, lhs_nz, rhs_nz);
        return b.create<mlir::arith::SelectOp>(loc, any, one, zero);
      }

      if (strcmp(fn, "&&") == 0) {
        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};

        if (!coerce_binary_numeric_operands(lhs, rhs, b, loc))
          return {};

        if (lhs.getType().isInteger(1))
          return b.create<mlir::arith::AndIOp>(loc, lhs, rhs);

        if (is_float_like(lhs.getType())) {
          auto zero = b.create<mlir::arith::ConstantOp>(
              loc, mlir::FloatAttr::get(lhs.getType(), 0.0));
          auto one = b.create<mlir::arith::ConstantOp>(
              loc, mlir::FloatAttr::get(lhs.getType(), 1.0));

          auto lhs_nz = b.create<mlir::arith::CmpFOp>(
              loc, mlir::arith::CmpFPredicate::ONE, lhs, zero);
          auto rhs_nz = b.create<mlir::arith::CmpFOp>(
              loc, mlir::arith::CmpFPredicate::ONE, rhs, zero);
          auto all = b.create<mlir::arith::AndIOp>(loc, lhs_nz, rhs_nz);
          return b.create<mlir::arith::SelectOp>(loc, all, one, zero);
        }

        auto zero = b.create<mlir::arith::ConstantOp>(
            loc, b.getIntegerAttr(lhs.getType(), 0));
        auto one = b.create<mlir::arith::ConstantOp>(
            loc, b.getIntegerAttr(lhs.getType(), 1));

        auto lhs_nz = b.create<mlir::arith::CmpIOp>(
            loc, mlir::arith::CmpIPredicate::ne, lhs, zero);
        auto rhs_nz = b.create<mlir::arith::CmpIOp>(
            loc, mlir::arith::CmpIPredicate::ne, rhs, zero);
        auto all = b.create<mlir::arith::AndIOp>(loc, lhs_nz, rhs_nz);
        return b.create<mlir::arith::SelectOp>(loc, all, one, zero);
      }

      if (strcmp(fn, "array_at") == 0) {

        mlir::Value array =
            dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx, ctx);

        mlir::Value idx =
            dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx, ctx);

        return b.create<dsp::ArrayAtOp>(loc, array, idx)->getResult(0);
      }

      if (strcmp(fn, "array_set") == 0) {

        mlir::Value array =
            dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx, ctx);

        mlir::Value idx =
            dsp_build_expr(ast->data.AST_APPLICATION.args + 1, dsp_ctx, ctx);

        mlir::Value val =
            dsp_build_expr(ast->data.AST_APPLICATION.args + 2, dsp_ctx, ctx);

        return b.create<dsp::ArraySetOp>(loc, array, idx, val)->getResult(0);
      }

      if (strcmp(fn, "Double") == 0) {

        mlir::Value val =
            dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx, ctx);
        return coerce_to_f64(val, b, loc);
      }

      if (strcmp(fn, "Int") == 0) {

        mlir::Value val =
            dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx, ctx);

        return coerce_to_i32(val, b, loc);
      }

      if (strcmp(fn, "wrap_upper") == 0) {
        mlir::Value lhs = dsp_build_expr(args, dsp_ctx, ctx);
        mlir::Value rhs = dsp_build_expr(args + 1, dsp_ctx, ctx);
        if (!lhs || !rhs)
          return {};

        lhs = coerce_to_f64(lhs, b, loc);
        rhs = coerce_to_f64(rhs, b, loc);

        auto module = dsp_ctx.frame_fn->getParentOfType<mlir::ModuleOp>();
        auto callee = get_or_create_runtime_decl(
            module, b, loc, "wrap_upper",
            b.getFunctionType({b.getF64Type(), b.getF64Type()},
                              {b.getF64Type()}));
        auto call = b.create<mlir::func::CallOp>(loc, callee,
                                                 mlir::ValueRange{lhs, rhs});
        return call.getResult(0);
      }
    }

    JITSymbol *callable_sym =
        lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);

    print_ast(ast);
    if (!callable_sym) {
      fprintf(stderr, "dsp_build_expr: unknown symbol\n");
      print_ast_err(ast);
      return {};
    }

    LLVMValueRef callable;
    if (callable_sym->type == STYPE_LAZY_EXTERN_FUNCTION) {
      auto module = dsp_ctx.frame_fn->getParentOfType<mlir::ModuleOp>();

      auto extern_fn = get_or_create_extern_decl(
          module, b, loc,
          callable_sym->symbol_data.STYPE_LAZY_EXTERN_FUNCTION.ast,
          callable_sym->symbol_type);

      if (!extern_fn) {
        fprintf(stderr,
                "dsp_build_expr: could not materialize extern fn decl\n");
        print_ast_err(ast);
        return {};
      }

      llvm::SmallVector<mlir::Value> call_args;
      call_args.reserve((size_t)nargs);
      Type *cur = callable_sym->symbol_type;
      for (int i = 0; i < nargs; ++i) {
        mlir::Value arg =
            dsp_build_expr(ast->data.AST_APPLICATION.args + i, dsp_ctx, ctx);
        if (!arg)
          return {};

        if (!cur || cur->kind != T_FN) {
          fprintf(stderr,
                  "dsp_build_expr: extern fn arg/type arity mismatch\n");
          print_ast_err(ast);
          return {};
        }

        auto expectedTy = mlir_type_from_ylc_type(cur->data.T_FN.from, b);

        if (!expectedTy) {
          return {};
        }
        if (expectedTy.isF64()) {
          arg = coerce_to_f64(arg, b, loc);
        }
        call_args.push_back(arg);
        cur = cur->data.T_FN.to;
      }

      auto call = b.create<mlir::func::CallOp>(loc, extern_fn, call_args);
      if (call.getNumResults() == 0) {
        printf("no results??\n");
        return {};
      }
      return call.getResult(0);
    }

    if (callable_sym->type == STYPE_AUDIO_JIT_SYM) {
      int synth_id = callable_sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
      MlirSynthRecord rec = mlir_registry_get(synth_id);
      if (!rec.name) {
        fprintf(stderr, "dsp_build_expr: missing MLIR synth record for call\n");
        print_ast_err(ast);
        return {};
      }

      llvm::SmallVector<mlir::Value> inputs;
      inputs.reserve((size_t)nargs);
      for (int i = 0; i < nargs; ++i) {
        mlir::Value arg =
            dsp_build_expr(ast->data.AST_APPLICATION.args + i, dsp_ctx, ctx);
        if (!arg) {
          return {};
        }
        inputs.push_back(coerce_to_f64(arg, b, loc));
      }

      return b
          .create<dsp::SubSynthOp>(loc, rec.name, rec.state_bytes,
                                   rec.num_inputs, inputs)
          .getResult();
    }
    printf("constructor??\n");
    print_ast(ast);

    return {};
  }

  case AST_LIST:
  case AST_ARRAY: {
    print_type(ast->type);
    print_ast(ast);

    mlir::Type elem_ty = mlir_array_elem_type_from_ylc(ast->type, b);

    if (!elem_ty) {
      fprintf(
          stderr,
          "dsp_build_expr: unsupported array element type in array literal\n");
      print_ast_err(ast);
      return {};
    }

    int len = ast->data.AST_LIST.len;
    llvm::SmallVector<mlir::Value> values;
    values.reserve((size_t)len);

    for (int i = 0; i < len; ++i) {
      mlir::Value v =
          dsp_build_expr(ast->data.AST_LIST.items + i, dsp_ctx, ctx);
      if (!v) {
        fprintf(stderr, "Error could not compile array value\n");
        print_ast_err(ast->data.AST_LIST.items + i);
        return {};
      }
      v = coerce_to_type(v, elem_ty, b, loc);
      if (!v || v.getType() != elem_ty) {
        fprintf(stderr, "dsp_build_expr: heterogeneous or unsupported array "
                        "literal element\n");
        print_ast_err(ast->data.AST_LIST.items + i);
        return {};
      }
      values.push_back(v);
    }

    return b.create<dsp::ArrayOp>(loc, elem_ty, values).getResult();
  }
  default:
    fprintf(stderr, "dsp_build_expr: unhandled tag %d\n", ast->tag);
    return {};
  }
}

// ---------------------------------------------------------------------------
// Entry point: build the real frame body, replacing the stub.
// frame_fn must already have its entry block (added by mlir_compile_synth).
// ---------------------------------------------------------------------------

bool dsp_build_frame(mlir::func::FuncOp frame_fn, mlir::OpBuilder &b,
                     mlir::Location loc, Ast *lambda, JITLangCtx *ctx,
                     int sample_rate) {
  // Clear whatever stub ops are in the entry block.
  frame_fn.getBody().front().clear();
  b.setInsertionPointToStart(&frame_fn.getBody().front());

  // frame signature: (ptr state, ptr node, f64 input_0, ...) -> f64
  // First two args are internal; expose only the f64 inputs to the body.
  mlir::Block &entry = frame_fn.getBody().front();
  int num_block_args = (int)entry.getNumArguments();

  JITLangCtx inner_ctx;
  STACK_ALLOC_CTX_PUSH(_frame_ctx, ctx)
  inner_ctx = _frame_ctx;

  // Bind lambda param names to the corresponding block arguments (offset
  // 2).
  int param_idx = 2;
  for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
    if (param_idx >= num_block_args)
      break;
    if (p->ast->tag == AST_IDENTIFIER) {
      const char *pname = p->ast->data.AST_IDENTIFIER.value;
      int plen = (int)p->ast->data.AST_IDENTIFIER.length;
      auto *v = new mlir::Value(entry.getArgument((unsigned)param_idx));
      JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, p->ast->type,
                                  mlir_val_to_sym_val(v), nullptr);
      ht_set_hash(inner_ctx.frame->table, pname,
                  hash_string(pname, (size_t)plen), sym);
      param_idx++;
    } else if (p->ast->tag == AST_TUPLE) {
      param_idx += (int)p->ast->data.AST_LIST.len;
    }
  }

  DspBuildCtx dsp_ctx{
      .frame_fn = frame_fn,
      .builder = b,
      .loc = loc,
      .sample_rate = sample_rate,
      .spf = 1.0 / (double)sample_rate,
  };

  mlir::Value result =
      dsp_build_expr(lambda->data.AST_LAMBDA.body, dsp_ctx, &inner_ctx);
  destroy_ctx(&inner_ctx);

  if (!result) {
    fprintf(stderr, "dsp_build_frame: body produced no value\n");
    return false;
  }

  result = coerce_to_f64(result, b, loc);
  b.create<mlir::func::ReturnOp>(loc, mlir::ValueRange{result});
  return true;
}
