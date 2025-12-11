# LLVM Coroutine Intrinsics Implementation Guide for YALCE

## Architecture Overview

### Coroutine Structure (Keep Current)
```c
struct CoroutineObject {
    i32 counter;      // Can repurpose: 0 = running, -1 = finished
    i8* fn_ptr;       // LLVM coroutine handle (from coro.begin)
    i8* state;        // Can be used for user data / closure captures
    i8* next;         // Chain to next coroutine (keep this feature)
    OptionT promise;  // The yielded value (keep this)
}
```

### Key Difference
- **Old**: `fn_ptr` points to your switch-based state machine function
- **New**: `fn_ptr` holds the LLVM coroutine handle (opaque i8* from `coro.begin`)
- **Old**: `counter` tracks which case in switch statement
- **New**: `counter` can track status (0 = active, -1 = done) or be repurposed

## Step-by-Step Implementation

### 1. Compile Coroutine Function (replaces `compile_coroutine`)

When you encounter a function with `yield` statements:

```c
LLVMValueRef compile_coroutine(Ast *expr, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {
    // expr is the function body with yield statements

    Type *yield_type = /* extract the yielded value type */;
    Type *option_type = create_option_type(yield_type);
    LLVMTypeRef promise_type = type_to_llvm_type(option_type, ctx, module);

    // 1. Create the coroutine function
    //    Signature: promise_type @coro_fn(args...)
    LLVMTypeRef coro_fn_type = LLVMFunctionType(
        promise_type,
        /* param types from expr */,
        /* num params */,
        0
    );

    LLVMValueRef coro_fn = LLVMAddFunction(module, "coro_body", coro_fn_type);

    // 2. Build the coroutine body
    LLVMBasicBlockRef entry_bb = LLVMAppendBasicBlock(coro_fn, "entry");
    LLVMBasicBlockRef cleanup_bb = LLVMAppendBasicBlock(coro_fn, "cleanup");
    LLVMBasicBlockRef suspend_bb = LLVMAppendBasicBlock(coro_fn, "suspend");

    LLVMPositionBuilderAtEnd(builder, entry_bb);

    // 3. Initialize coroutine intrinsics

    // Get intrinsic function declarations
    LLVMValueRef coro_id_fn = LLVMGetNamedFunction(module, "llvm.coro.id");
    if (!coro_id_fn) {
        LLVMTypeRef coro_id_type = LLVMFunctionType(
            LLVMTokenType(),
            (LLVMTypeRef[]){
                LLVMInt32Type(),  // align
                GENERIC_PTR,      // promise
                GENERIC_PTR,      // coroaddr
                GENERIC_PTR       // fnaddr
            },
            4, 0
        );
        coro_id_fn = LLVMAddFunction(module, "llvm.coro.id", coro_id_type);
    }

    LLVMValueRef coro_begin_fn = LLVMGetNamedFunction(module, "llvm.coro.begin");
    if (!coro_begin_fn) {
        LLVMTypeRef coro_begin_type = LLVMFunctionType(
            GENERIC_PTR,
            (LLVMTypeRef[]){LLVMTokenType(), GENERIC_PTR},
            2, 0
        );
        coro_begin_fn = LLVMAddFunction(module, "llvm.coro.begin", coro_begin_type);
    }

    LLVMValueRef coro_size_fn = LLVMGetNamedFunction(module, "llvm.coro.size.i64");
    if (!coro_size_fn) {
        LLVMTypeRef coro_size_type = LLVMFunctionType(LLVMInt64Type(), NULL, 0, 0);
        coro_size_fn = LLVMAddFunction(module, "llvm.coro.size.i64", coro_size_type);
    }

    // 4. Call coro.id
    LLVMValueRef id = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(coro_id_fn), coro_id_fn,
        (LLVMValueRef[]){
            LLVMConstInt(LLVMInt32Type(), 0, 0),  // align
            LLVMConstNull(GENERIC_PTR),           // promise (we manage separately)
            LLVMConstNull(GENERIC_PTR),           // coroaddr
            LLVMConstNull(GENERIC_PTR)            // fnaddr
        },
        4, "coro.id"
    );

    // 5. Allocate coroutine frame
    LLVMValueRef size = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(coro_size_fn), coro_size_fn,
        NULL, 0, "coro.size"
    );

    LLVMValueRef frame = LLVMBuildArrayMalloc(builder, LLVMInt8Type(), size, "coro.frame");

    // 6. Call coro.begin
    LLVMValueRef handle = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(coro_begin_fn), coro_begin_fn,
        (LLVMValueRef[]){id, frame},
        2, "coro.handle"
    );

    // 7. Build coroutine body with yields
    //    This is where you codegen the actual function body
    //    When you hit a yield, you'll emit coro.suspend

    // Example structure:
    CoroutineCtx coro_ctx = {
        .handle = handle,
        .promise_type = promise_type,
        .cleanup_bb = cleanup_bb,
        .suspend_bb = suspend_bb
    };

    JITLangCtx coro_lang_ctx = *ctx;
    coro_lang_ctx.coro_ctx = &coro_ctx;

    // Codegen the body - yields will use codegen_yield which emits coro.suspend
    codegen(expr->data.AST_LAMBDA.body, &coro_lang_ctx, module, builder);

    // 8. Final suspend (when function ends naturally)
    LLVMBuildBr(builder, suspend_bb);

    // 9. Suspend block
    LLVMPositionBuilderAtEnd(builder, suspend_bb);

    LLVMValueRef coro_end_fn = get_or_declare_intrinsic(module, "llvm.coro.end",
        LLVMFunctionType(LLVMInt1Type(),
            (LLVMTypeRef[]){GENERIC_PTR, LLVMInt1Type()}, 2, 0));

    LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_end_fn), coro_end_fn,
        (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)},
        2, "");

    // Return None to indicate completion
    LLVMValueRef none_value = create_option_none(promise_type, builder);
    LLVMBuildRet(builder, none_value);

    // 10. Cleanup block (for early termination)
    LLVMPositionBuilderAtEnd(builder, cleanup_bb);
    LLVMValueRef coro_free_fn = get_or_declare_intrinsic(module, "llvm.coro.free",
        LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){LLVMTokenType(), GENERIC_PTR}, 2, 0));

    LLVMValueRef mem = LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_free_fn),
        coro_free_fn, (LLVMValueRef[]){id, handle}, 2, "coro.free");

    LLVMBuildFree(builder, mem);
    LLVMBuildBr(builder, suspend_bb);

    return coro_fn;
}
```

### 2. Implement Yield (replaces `codegen_yield`)

```c
LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx,
                          LLVMModuleRef module, LLVMBuilderRef builder) {
    CoroutineCtx *coro_ctx = (CoroutineCtx *)ctx->coro_ctx;

    // 1. Codegen the value being yielded
    LLVMValueRef yield_value = codegen(ast->data.AST_YIELD.value, ctx, module, builder);

    // 2. If yielding a nested coroutine, we need special handling
    //    (Keep your current chaining logic for this)

    // 3. Create the Option::Some(value) to return
    LLVMValueRef some_value = create_option_some(
        yield_value,
        ast->data.AST_YIELD.value->type,
        coro_ctx->promise_type,
        builder
    );

    // 4. Emit suspension point
    LLVMValueRef coro_save_fn = get_or_declare_intrinsic(module, "llvm.coro.save",
        LLVMFunctionType(LLVMTokenType(), (LLVMTypeRef[]){GENERIC_PTR}, 1, 0));

    LLVMValueRef save_token = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(coro_save_fn), coro_save_fn,
        (LLVMValueRef[]){coro_ctx->handle},
        1, "coro.save"
    );

    LLVMValueRef coro_suspend_fn = get_or_declare_intrinsic(module, "llvm.coro.suspend",
        LLVMFunctionType(LLVMInt8Type(),
            (LLVMTypeRef[]){LLVMTokenType(), LLVMInt1Type()}, 2, 0));

    LLVMValueRef suspend_result = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(coro_suspend_fn), coro_suspend_fn,
        (LLVMValueRef[]){
            save_token,
            LLVMConstInt(LLVMInt1Type(), 0, 0)  // not final
        },
        2, "coro.suspend"
    );

    // 5. Switch on suspension result
    //    -1 = coroutine finished (shouldn't happen on yield)
    //     0 = coroutine suspended (normal case)
    //     1 = coroutine destroyed (cleanup needed)

    LLVMBasicBlockRef suspend_bb = LLVMAppendBasicBlock(
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)),
        "yield.suspend"
    );
    LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)),
        "yield.resume"
    );
    LLVMBasicBlockRef cleanup_bb = coro_ctx->cleanup_bb;

    LLVMValueRef switch_inst = LLVMBuildSwitch(builder, suspend_result, suspend_bb, 2);
    LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), suspend_bb);
    LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

    // 6. Suspend block - return the yielded value
    LLVMPositionBuilderAtEnd(builder, suspend_bb);
    LLVMBuildRet(builder, some_value);

    // 7. Resume block - execution continues here after resume
    LLVMPositionBuilderAtEnd(builder, resume_bb);

    // Yield expression evaluates to unit/void
    return LLVMConstNull(LLVMVoidType());
}
```

### 3. Create Coroutine Constructor (replaces `coro_create`)

This creates your wrapper object when a coroutine function is called:

```c
LLVMValueRef coro_create(JITSymbol *sym, Type *expected_fn_type, Ast *app,
                         JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {

    // sym->val is the compiled coroutine function
    LLVMValueRef coro_fn = sym->val;

    // 1. Get promise type
    Type *yield_type = fn_return_type(expected_fn_type);
    Type *option_type = create_option_type(yield_type);
    LLVMTypeRef promise_type = type_to_llvm_type(option_type, ctx, module);

    // 2. Build coroutine object type
    LLVMTypeRef coro_obj_type = CORO_OBJ_TYPE(promise_type);

    // 3. Call the coroutine function to get the handle
    //    Codegen arguments
    LLVMValueRef *args = /* codegen application args */;
    int num_args = /* number of args */;

    LLVMValueRef first_call = LLVMBuildCall2(
        builder,
        LLVMGlobalGetValueType(coro_fn),
        coro_fn,
        args,
        num_args,
        "coro.init"
    );

    // The first call returns Some(first_value) or None, but we want the handle
    // We need to modify the approach slightly...

    // BETTER APPROACH: Create a wrapper that returns the handle
    // The coroutine function should have TWO entry points:
    // 1. Initial call - sets up and returns handle
    // 2. Resume - continues execution

    // For now, let's use a different strategy:
    // Store the coroutine function pointer and args in the state

    // 4. Allocate coroutine wrapper object
    LLVMValueRef coro_obj = LLVMBuildMalloc(builder, coro_obj_type, "coro_obj");

    // 5. Initialize fields
    //    counter = 0 (not started yet / active)
    LLVMBuildStore(builder,
        LLVMConstInt(LLVMInt32Type(), 0, 0),
        LLVMBuildStructGEP2(builder, coro_obj_type, coro_obj,
            CORO_COUNTER_SLOT, "counter_gep")
    );

    //    fn_ptr = coroutine handle (we'll get this on first resume)
    //    For LLVM coroutines, we need to rethink this...
    //    Actually store the coroutine function + captured state

    //    state = captured arguments/closure
    LLVMValueRef state_struct = /* build struct with captured args */;
    LLVMBuildStore(builder, state_struct,
        coro_state_gep(coro_obj, coro_obj_type, builder));

    //    next = NULL
    LLVMBuildStore(builder, LLVMConstNull(GENERIC_PTR),
        LLVMBuildStructGEP2(builder, coro_obj_type, coro_obj,
            CORO_NEXT_SLOT, "next_gep"));

    //    promise = None (initially)
    LLVMValueRef none = create_option_none(promise_type, builder);
    LLVMBuildStore(builder, none,
        coro_promise_gep(coro_obj, coro_obj_type, builder));

    return coro_obj;
}
```

### 4. Resume Coroutine (replaces `coro_resume`/`coro_advance`)

```c
LLVMValueRef coro_resume(JITSymbol *sym, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {

    // This is called when the user invokes the coroutine instance: `co()`

    // Build a wrapper function: OptionT resume_wrapper(CoroutineObj* coro)

    Type *yield_type = /* extract from sym */;
    Type *option_type = create_option_type(yield_type);
    LLVMTypeRef promise_type = type_to_llvm_type(option_type, ctx, module);
    LLVMTypeRef coro_obj_type = CORO_OBJ_TYPE(promise_type);

    LLVMTypeRef wrapper_type = LLVMFunctionType(
        promise_type,
        (LLVMTypeRef[]){LLVMPointerType(coro_obj_type, 0)},
        1, 0
    );

    LLVMValueRef wrapper_fn = LLVMAddFunction(module, "coro_resume_wrapper", wrapper_type);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(wrapper_fn, "entry");
    LLVMBasicBlockRef first_call = LLVMAppendBasicBlock(wrapper_fn, "first_call");
    LLVMBasicBlockRef subsequent = LLVMAppendBasicBlock(wrapper_fn, "subsequent");
    LLVMBasicBlockRef is_done = LLVMAppendBasicBlock(wrapper_fn, "is_done");

    LLVMBasicBlockRef prev_bb = LLVMGetInsertBlock(builder);
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef coro_obj_arg = LLVMGetParam(wrapper_fn, 0);

    // 1. Check if this is first call (counter == 0 and fn_ptr == NULL)
    LLVMValueRef fn_ptr = LLVMBuildLoad2(builder, GENERIC_PTR,
        LLVMBuildStructGEP2(builder, coro_obj_type, coro_obj_arg,
            CORO_FN_PTR_SLOT, "fn_ptr_gep"),
        "fn_ptr");

    LLVMValueRef is_first = LLVMBuildIsNull(builder, fn_ptr, "is_first");
    LLVMBuildCondBr(builder, is_first, first_call, subsequent);

    // 2. First call - initialize coroutine
    LLVMPositionBuilderAtEnd(builder, first_call);

    // Call the coroutine function with captured state
    // This requires restructuring to store function + args in state

    // Get handle from first invocation...
    // CHALLENGE: LLVM coroutines don't expose handle this way

    // ALTERNATIVE: Generate resume function alongside coroutine
    // Store resume function pointer in fn_ptr field

    LLVMBuildBr(builder, subsequent);

    // 3. Subsequent calls - resume
    LLVMPositionBuilderAtEnd(builder, subsequent);

    // Get the coroutine handle
    LLVMValueRef handle = LLVMBuildLoad2(builder, GENERIC_PTR,
        LLVMBuildStructGEP2(builder, coro_obj_type, coro_obj_arg,
            CORO_FN_PTR_SLOT, ""),
        "handle");

    // Check if done
    LLVMValueRef coro_done_fn = get_or_declare_intrinsic(module, "llvm.coro.done",
        LLVMFunctionType(LLVMInt1Type(), (LLVMTypeRef[]){GENERIC_PTR}, 1, 0));

    LLVMValueRef done = LLVMBuildCall2(builder,
        LLVMGlobalGetValueType(coro_done_fn), coro_done_fn,
        (LLVMValueRef[]){handle}, 1, "is_done");

    LLVMBuildCondBr(builder, done, is_done, /* not_done */ );

    // Not done - resume
    LLVMValueRef coro_resume_fn = get_or_declare_intrinsic(module, "llvm.coro.resume",
        LLVMFunctionType(LLVMVoidType(), (LLVMTypeRef[]){GENERIC_PTR}, 1, 0));

    LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_resume_fn),
        coro_resume_fn, (LLVMValueRef[]){handle}, 1, "");

    // The resume will jump back into the coroutine and it will return a value
    // But how do we get that value?

    // INSIGHT: We need to use coro.promise to get the yielded value
    // Set up promise storage before calling resume

    LLVMBuildRet(builder, /* return promise */);

    // Done block
    LLVMPositionBuilderAtEnd(builder, is_done);
    LLVMValueRef none = create_option_none(promise_type, builder);
    LLVMBuildRet(builder, none);

    LLVMPositionBuilderAtEnd(builder, prev_bb);
    return wrapper_fn;
}
```

## Critical Insight: LLVM Coroutines are Different

After working through this, I realize **LLVM's coroutine intrinsics have a fundamentally different model** than your current implementation:

### Your Model (More like Python generators)
1. Calling coroutine function returns a coroutine object
2. Calling `coro()` advances it and returns the next value
3. Function naturally returns values at yield points

### LLVM Model (More like C++20 coroutines)
1. Coroutine is the entire function with suspend points
2. Calling coroutine creates a "promise" object and coroutine frame
3. Resume/destroy are external operations on the handle
4. Values are communicated through the promise object

## Recommended Hybrid Approach

Keep your current wrapper structure but use LLVM intrinsics internally:

```
┌─────────────────────────────────┐
│   Your CoroutineObject          │
│  ┌────────────────────────────┐ │
│  │ counter, next, promise     │ │  <- Keep for chaining, state
│  └────────────────────────────┘ │
│  ┌────────────────────────────┐ │
│  │ LLVM Coroutine Handle      │ │  <- LLVM manages suspend/resume
│  │ (from coro.begin)          │ │
│  └────────────────────────────┘ │
└─────────────────────────────────┘
```

### Implementation Strategy

1. **Coroutine Function**: Emit using LLVM intrinsics with yields as suspend points

2. **Wrapper Function**: Create a "resume wrapper" that:
   - Holds the LLVM coroutine handle
   - Manages promise value transfer
   - Provides your `() -> Option T` interface

3. **Constructor**: When coroutine function is called:
   - Generate the coroutine with args embedded
   - Store handle in wrapper object
   - Return wrapper object

4. **Resume**: When wrapper is called:
   - Call `llvm.coro.resume` on handle
   - Extract value from promise
   - Return as Option type

##Next Steps

Would you like me to:

1. **Implement a complete working example** - A full implementation of one simple coroutine using LLVM intrinsics that you can test

2. **Create helper functions** - Build out the intrinsic declaration helpers and wrapper generators

3. **Show the promise mechanism** - Demonstrate how to use `coro.promise` to pass values between coroutine and caller

4. **Handle your chaining semantics** - Show how to preserve `yield nested_coro()` behavior with LLVM intrinsics

Let me know which direction would be most helpful!
