# Complete LLVM Coroutine Implementation Example

This document shows a complete working example of how to implement the Fibonacci coroutine from your test suite using LLVM coroutine intrinsics.

## Test Case

From `test/test_scripts/10_coroutines.ylc`:

```ocaml
let fib = fn a b ->
  yield a;
  yield fib b (a + b)
;;

let co = fib 0 1 in
   co () == Some 0
&& co () == Some 1
&& co () == Some 1
&& co () == Some 2
&& co () == Some 3
&& co () == Some 5
&& co () == Some 8
```

## Architecture Overview

```
┌──────────────────────────────────────────────────────────┐
│ YALCE User Code: let co = fib 0 1 in co()              │
└──────────────────────────────────────────────────────────┘
                          │
                          ▼
┌──────────────────────────────────────────────────────────┐
│ Closure: { coro_obj*, resume_wrapper_fn }               │
│  Provides: () -> Option<T> interface                     │
└──────────────────────────────────────────────────────────┘
                          │
                          ▼
┌──────────────────────────────────────────────────────────┐
│ Resume Wrapper Function                                  │
│  - Checks if done (coro.done)                           │
│  - Calls coro.resume                                     │
│  - Returns promise value                                 │
│  - Handles chaining (checks `next` field)               │
└──────────────────────────────────────────────────────────┘
                          │
                          ▼
┌──────────────────────────────────────────────────────────┐
│ LLVM Coroutine Handle (i8*)                             │
│  LLVM manages:                                           │
│   - State machine (yield points)                        │
│   - Variable persistence                                 │
│   - Frame allocation                                     │
└──────────────────────────────────────────────────────────┘
                          │
                          ▼
┌──────────────────────────────────────────────────────────┐
│ Coroutine Body Function                                  │
│  - Contains yield statements                             │
│  - Variables automatically persist across yields         │
│  - LLVM handles suspend/resume                          │
└──────────────────────────────────────────────────────────┘
```

## Step-by-Step Implementation

### Step 1: Detect Coroutine Functions

In your type inference or AST analysis, mark functions containing `yield` statements:

```c
bool contains_yield(Ast *expr) {
    if (expr->tag == AST_YIELD) {
        return true;
    }

    // Recursively check children
    // ... implementation details

    return false;
}
```

### Step 2: Compile Coroutine Functions

When compiling a function that contains yields:

```c
LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder) {

    // Check if this is a coroutine function
    if (contains_yield(ast->data.AST_LAMBDA.body)) {
        // Use LLVM intrinsics implementation
        return compile_coroutine(ast, ctx, module, builder);
    }

    // Regular function compilation
    return compile_regular_function(ast, ctx, module, builder);
}
```

### Step 3: Handle Coroutine Calls

When a coroutine function is called, create the wrapper:

```c
// In application.c or similar
LLVMValueRef codegen_application(Ast *app, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

    JITSymbol *fn_sym = /* lookup function symbol */;

    if (fn_sym->type == STYPE_COROUTINE_CONSTRUCTOR) {
        // Create coroutine instance
        return coro_create(fn_sym, app->type, app, ctx, module, builder);
    }

    // Regular function call
    return /* normal application */;
}
```

### Step 4: Example LLVM IR Output

For the Fibonacci example, LLVM generates something like:

```llvm
; Coroutine body function
define { i8*, {i8, [8 x i8]}* } @coro_body_0() {
entry:
  ; Initialize coroutine
  %coro.id = call token @llvm.coro.id(i32 0, i8* null, i8* null, i8* null)
  %coro.size = call i64 @llvm.coro.size.i64()
  %coro.frame = call i8* @malloc(i64 %coro.size)
  %coro.handle = call i8* @llvm.coro.begin(token %coro.id, i8* %coro.frame)

  ; Allocate promise storage
  %promise = alloca {i8, [8 x i8]}

  ; Function body with yields...
  ; Variables a, b are automatically preserved by LLVM across yields

  ; First yield: yield a
  %some_a = insertvalue {i8, [8 x i8]} undef, i8 0, 0  ; tag = 0 (Some)
  ; ... insert value ...
  store {i8, [8 x i8]} %some_a, {i8, [8 x i8]}* %promise
  %save1 = call token @llvm.coro.save(i8* %coro.handle)
  %suspend1 = call i8 @llvm.coro.suspend(token %save1, i1 false)
  switch i8 %suspend1, label %suspend [
    i8 0, label %suspend
    i8 1, label %cleanup
  ]

resume_after_yield_1:
  ; Variables a, b are still available here!
  ; Second yield: yield fib b (a + b)
  ; ... recursive call and yield ...

suspend:
  call i1 @llvm.coro.end(i8* %coro.handle, i1 false)
  %result = insertvalue { i8*, {i8, [8 x i8]}* } undef, i8* %coro.handle, 0
  %result2 = insertvalue { i8*, {i8, [8 x i8]}* } %result, {i8, [8 x i8]}* %promise, 1
  ret { i8*, {i8, [8 x i8]}* } %result2

cleanup:
  %mem = call i8* @llvm.coro.free(token %coro.id, i8* %coro.handle)
  call void @free(i8* %mem)
  br label %suspend
}

; Resume wrapper function
define {i8, [8 x i8]} @coro_resume_wrapper_0({ i32, i8*, i8*, i8*, {i8, [8 x i8]} }* %coro_obj) {
entry:
  ; Load handle from coroutine object
  %handle_gep = getelementptr { i32, i8*, i8*, i8*, {i8, [8 x i8]} },
                               { i32, i8*, i8*, i8*, {i8, [8 x i8]} }* %coro_obj,
                               i32 0, i32 1
  %handle = load i8*, i8** %handle_gep

  ; Check if done
  %is_done = call i1 @llvm.coro.done(i8* %handle)
  br i1 %is_done, label %done, label %resume

done:
  ; Return None
  %none = insertvalue {i8, [8 x i8]} undef, i8 1, 0
  ret {i8, [8 x i8]} %none

resume:
  ; Resume coroutine
  call void @llvm.coro.resume(i8* %handle)

  ; Load promise value
  %promise_gep = getelementptr { i32, i8*, i8*, i8*, {i8, [8 x i8]} },
                                { i32, i8*, i8*, i8*, {i8, [8 x i8]} }* %coro_obj,
                                i32 0, i32 4
  %promise = load {i8, [8 x i8]}, {i8, [8 x i8]}* %promise_gep
  ret {i8, [8 x i8]} %promise
}
```

## Handling Variables Across Yields

One of the key advantages of LLVM intrinsics is automatic variable persistence:

```ocaml
let example = fn () ->
  let x = expensive_computation();
  let y = another_computation();
  yield x;
  (* LLVM automatically preserves x and y here *)
  yield (x + y);  (* Both x and y are still available! *)
;;
```

The LLVM coroutine passes analyze which variables are live across suspension points and automatically include them in the coroutine frame.

### Manual Approach (Old)

In your old implementation, you had to manually identify these variables:

```c
// From old code
LLVMTypeRef get_coro_state_layout(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module) {
    // Manually find all variables that cross yield boundaries
    // Build a struct to hold them
    // ...
}
```

### LLVM Approach (New)

With LLVM intrinsics:

```c
// No manual analysis needed!
// Just codegen the function body normally
// LLVM's coroutine passes do the analysis
LLVMValueRef result = codegen_lambda_body(expr, coro_ctx, module, builder);
```

## Nested Coroutine Chaining

For `yield nested_coro()`, you have two options:

### Option 1: Trampoline in Wrapper

The resume wrapper handles chaining:

```c
LLVMValueRef build_resume_wrapper(...) {
    // ...

    // After resuming outer coroutine, check promise value
    // If it's a coroutine type, store it in `next` field
    // On next call, resume `next` first

    check_if_promise_is_coroutine_block:
        // If promise contains a coroutine:
        // 1. Extract coroutine object from promise
        // 2. Store in coro_obj->next field
        // 3. Immediately call resume on it
        // 4. Return its value

    // When nested coroutine finishes (returns None):
    // 1. Clear next field
    // 2. Resume outer coroutine
}
```

### Option 2: Recursive Handling in Codegen

```c
LLVMValueRef codegen_yield(Ast *ast, ...) {
    LLVMValueRef yield_value = codegen(ast->data.AST_YIELD.expr, ...);
    Type *yield_val_type = ast->data.AST_YIELD.expr->type;

    if (is_coroutine_type(yield_val_type)) {
        // Instead of yielding the coroutine object,
        // yield a special marker that tells wrapper
        // to look in a designated location for the nested coroutine

        // Store nested coroutine in a field accessible by wrapper
        // (e.g., pass coroutine object as param to coroutine function)
    }

    // Regular yield
    // ...
}
```

## Integration Steps

To integrate this into your current codebase:

1. **Add new source files to build**:
   ```makefile
   COROUTINE_SOURCES = \
       lang/backend_llvm/coroutines_llvm_intrinsics.c \
       lang/backend_llvm/coroutines_wrapper.c \
       lang/backend_llvm/coroutines_chaining.c
   ```

2. **Update coroutines.c to use new implementation**:
   ```c
   // In coroutines.c
   #ifdef USE_LLVM_CORO_INTRINSICS
   #include "coroutines_llvm_intrinsics.c"
   #include "coroutines_wrapper.c"
   #else
   // Keep old switch-based implementation
   #endif
   ```

3. **Add compiler flag to toggle implementation**:
   ```bash
   make CFLAGS="-DUSE_LLVM_CORO_INTRINSICS" ylc
   ```

4. **Test with existing coroutine tests**:
   ```bash
   make test_scripts
   ```

## Debugging Tips

### View Generated IR

```bash
# Dump LLVM IR before optimization
LLVMDumpModule(module);

# Or write to file
LLVMPrintModuleToFile(module, "coroutine.ll", &error);
```

### Check Coroutine Passes

LLVM requires specific optimization passes to lower coroutines:

```c
// Make sure these are in your optimization pipeline:
LLVMPassManagerBuilderRef pmb = LLVMPassManagerBuilderCreate();
LLVMPassManagerBuilderSetOptLevel(pmb, 2);

LLVMPassManagerRef pm = LLVMCreatePassManager();
LLVMPassManagerBuilderPopulateModulePassManager(pmb, pm);

// Coroutine passes are added automatically at opt level 2+
LLVMRunPassManager(pm, module);
```

### Common Issues

1. **"coro.begin must be preceded by coro.id"**
   - Make sure you call coro.id before coro.begin

2. **Variables not persisting across yields**
   - Check that optimization passes are running
   - Variables must be SSA values, not allocas

3. **Crashes on resume**
   - Ensure coro.end is called in all exit paths
   - Check that cleanup block frees memory correctly

## Performance Considerations

### Memory Usage

- Each coroutine frame is heap-allocated
- Size determined by live variables across yields
- Consider arena allocator for many short-lived coroutines

### Optimization

LLVM's coroutine passes can optimize:
- Eliminate unused variables from frame
- Merge suspension points
- Inline small coroutines

### Comparison to Manual Implementation

| Aspect | Manual Switch | LLVM Intrinsics |
|--------|--------------|-----------------|
| Code Size | Larger | Smaller |
| Compile Time | Faster | Slower (optimization passes) |
| Runtime Speed | Similar | Similar |
| Maintainability | Complex | Simpler |
| Variable Tracking | Manual | Automatic |

## Next Steps

1. Implement coroutine with arguments:
   ```ocaml
   let counter = fn start step ->
     let current = start in
     yield current;
     yield counter (start + step) step
   ;;
   ```

2. Add error handling for malformed coroutines

3. Implement coroutine combinators:
   ```ocaml
   let cor_map : (a -> b) -> Coroutine a -> Coroutine b
   let cor_filter : (a -> bool) -> Coroutine a -> Coroutine a
   let cor_take : int -> Coroutine a -> Coroutine a
   ```

4. Optimize for common cases (e.g., coroutines with no variables)

## Additional Resources

- [LLVM Coroutine Documentation](https://llvm.org/docs/Coroutines.html)
- [LLVM Coroutine Intrinsics Reference](https://llvm.org/docs/LangRef.html#coroutine-intrinsics)
- [C++ Coroutines using LLVM (example)](https://github.com/GorNishanov/llvm/wiki)
- [Understanding Coroutine Theory](https://lewissbaker.github.io/2017/09/25/coroutine-theory)
