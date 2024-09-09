; ModuleID = 'variant_ctx.c'
source_filename = "variant_ctx.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx13.0.0"

%struct.TU = type { i32, %union.anon }
%union.anon = type { double }

@__const.main.t1 = private unnamed_addr constant { i32, [4 x i8], { i32, [4 x i8] } } { i32 0, [4 x i8] undef, { i32, [4 x i8] } { i32 1, [4 x i8] undef } }, align 8
@__const.main.t2 = private unnamed_addr constant { i32, [4 x i8], { i32, [4 x i8] } } { i32 0, [4 x i8] undef, { i32, [4 x i8] } { i32 3, [4 x i8] undef } }, align 8
@__const.main.t3 = private unnamed_addr constant %struct.TU { i32 1, %union.anon { double 2.000000e+00 } }, align 8
@__const.main.t4 = private unnamed_addr constant { i32, { i64 } } { i32 2, { i64 } { i64 200 } }, align 8

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @proc_tu([2 x i64] %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca %struct.TU, align 8
  store [2 x i64] %0, ptr %3, align 8
  %4 = getelementptr inbounds %struct.TU, ptr %3, i32 0, i32 0
  %5 = load i32, ptr %4, align 8
  switch i32 %5, label %14 [
    i32 0, label %6
    i32 1, label %12
    i32 2, label %13
  ]

6:                                                ; preds = %1
  %7 = getelementptr inbounds %struct.TU, ptr %3, i32 0, i32 1
  %8 = load i32, ptr %7, align 8
  %9 = icmp eq i32 %8, 3
  br i1 %9, label %10, label %11

10:                                               ; preds = %6
  store i32 4, ptr %2, align 4
  br label %14

11:                                               ; preds = %6
  store i32 1, ptr %2, align 4
  br label %14

12:                                               ; preds = %1
  store i32 2, ptr %2, align 4
  br label %14

13:                                               ; preds = %1
  store i32 3, ptr %2, align 4
  br label %14

14:                                               ; preds = %10, %11, %12, %13, %1
  %15 = load i32, ptr %2, align 4
  ret i32 %15
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @main() #0 {
  %1 = alloca %struct.TU, align 8
  %2 = alloca %struct.TU, align 8
  %3 = alloca %struct.TU, align 8
  %4 = alloca %struct.TU, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %1, ptr align 8 @__const.main.t1, i64 16, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 8 @__const.main.t2, i64 16, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %3, ptr align 8 @__const.main.t3, i64 16, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %4, ptr align 8 @__const.main.t4, i64 16, i1 false)
  %5 = load [2 x i64], ptr %1, align 8
  %6 = call i32 @proc_tu([2 x i64] %5)
  %7 = load [2 x i64], ptr %2, align 8
  %8 = call i32 @proc_tu([2 x i64] %7)
  %9 = load [2 x i64], ptr %3, align 8
  %10 = call i32 @proc_tu([2 x i64] %9)
  %11 = load [2 x i64], ptr %4, align 8
  %12 = call i32 @proc_tu([2 x i64] %11)
  ret i32 0
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #1

attributes #0 = { noinline nounwind optnone ssp uwtable(sync) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+crypto,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+sha3,+sm4,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8.5a,+v8a,+zcm,+zcz" }
attributes #1 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"uwtable", i32 1}
!3 = !{i32 7, !"frame-pointer", i32 1}
!4 = !{!"Homebrew clang version 16.0.6"}
