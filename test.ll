; ModuleID = 'ylc.top-level'
source_filename = "cor_test.ylc"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"

@global_storage_array = external local_unnamed_addr global [1024 x ptr]

define void @top() local_unnamed_addr #0 {
entry:
  %coro.frame.i = tail call ptr @malloc(i32 32)
  store ptr @coro_c.resume, ptr %coro.frame.i, align 8
  %destroy.addr.i = getelementptr inbounds nuw i8, ptr %coro.frame.i, i64 8
  store ptr @coro_c.destroy, ptr %destroy.addr.i, align 8
  %0 = getelementptr inbounds nuw i8, ptr %coro.frame.i, i64 16
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 4 dereferenceable(9) %0, i8 0, i64 9, i1 false)
  %coro.frame.i42 = tail call ptr @malloc(i32 40)
  store ptr @coro_loop_wrapper_0.resume, ptr %coro.frame.i42, align 8
  %destroy.addr.i44 = getelementptr inbounds nuw i8, ptr %coro.frame.i42, i64 8
  store ptr @coro_loop_wrapper_0.destroy, ptr %destroy.addr.i44, align 8
  %1 = getelementptr inbounds nuw i8, ptr %coro.frame.i42, i64 16
  store i64 0, ptr %1, align 4
  %inner_coro.param.spill.addr.i = getelementptr inbounds nuw i8, ptr %coro.frame.i42, i64 24
  store ptr %coro.frame.i, ptr %inner_coro.param.spill.addr.i, align 8
  %index.addr12.i = getelementptr inbounds nuw i8, ptr %coro.frame.i42, i64 32
  store i1 false, ptr %index.addr12.i, align 1
  %x_malloc = tail call ptr @malloc(i32 8)
  store ptr %coro.frame.i42, ptr %x_malloc, align 8
  store ptr %x_malloc, ptr @global_storage_array, align 8
  %get_is_done_flag = getelementptr inbounds nuw i8, ptr %coro.frame.i42, i64 20
  %2 = load i1, ptr %get_is_done_flag, align 1
  br i1 %2, label %coro.merge30, label %yield_from.check.ithread-pre-split

yield_from.check.ithread-pre-split:               ; preds = %entry, %yield_from.body.i
  %inner_coro.param.reload9.i.pre.ph = phi ptr [ %inner_coro.param.reload9.i.pre.pre, %yield_from.body.i ], [ %coro.frame.i, %entry ]
  %.pr = load ptr, ptr %inner_coro.param.reload9.i.pre.ph, align 8
  %3 = icmp eq ptr %.pr, null
  br i1 %3, label %yield_from.check.i, label %yield_from.body.i

yield_from.check.i:                               ; preds = %yield_from.check.ithread-pre-split, %yield_from.check.i
  br label %yield_from.check.i, !llvm.loop !0

yield_from.body.i:                                ; preds = %yield_from.check.ithread-pre-split
  %4 = load ptr, ptr %inner_coro.param.reload9.i.pre.ph, align 8
  tail call fastcc void %4(ptr nonnull %inner_coro.param.reload9.i.pre.ph)
  %5 = load ptr, ptr %inner_coro.param.reload9.i.pre.ph, align 8
  %6 = icmp eq ptr %5, null
  %inner_coro.param.reload9.i.pre.pre = load ptr, ptr %inner_coro.param.spill.addr.i, align 8
  br i1 %6, label %yield_from.check.ithread-pre-split, label %coro.merge

coro.merge:                                       ; preds = %yield_from.body.i
  %7 = getelementptr inbounds nuw i8, ptr %inner_coro.param.reload9.i.pre.pre, i64 16
  %inner.value.i = load i32, ptr %7, align 4
  store i32 %inner.value.i, ptr %1, align 4
  store i1 true, ptr %index.addr12.i, align 1
  %.pre = load i1, ptr %get_is_done_flag, align 1
  %.pre45 = load ptr, ptr %coro.frame.i42, align 8
  %8 = icmp eq ptr %.pre45, null
  %9 = or i1 %.pre, i1 %8
  br i1 %9, label %coro.merge4, label %yield_from.check.i53.outer

yield_from.check.i53.outer:                       ; preds = %coro.merge, %yield_from.body.i56
  %inner_coro.param.reload9.i55.pre85.ph = phi ptr [ %inner_coro.param.reload9.i55.pre.pre, %yield_from.body.i56 ], [ %inner_coro.param.reload9.i.pre.pre, %coro.merge ]
  %10 = load ptr, ptr %inner_coro.param.reload9.i55.pre85.ph, align 8
  %11 = icmp eq ptr %10, null
  br label %yield_from.check.i53

yield_from.check.i53:                             ; preds = %yield_from.check.i53.outer, %yield_from.check.i53
  br i1 %11, label %yield_from.check.i53, label %yield_from.body.i56

yield_from.body.i56:                              ; preds = %yield_from.check.i53
  %12 = load ptr, ptr %inner_coro.param.reload9.i55.pre85.ph, align 8
  tail call fastcc void %12(ptr nonnull %inner_coro.param.reload9.i55.pre85.ph)
  %13 = load ptr, ptr %inner_coro.param.reload9.i55.pre85.ph, align 8
  %14 = icmp eq ptr %13, null
  %inner_coro.param.reload9.i55.pre.pre = load ptr, ptr %inner_coro.param.spill.addr.i, align 8
  br i1 %14, label %yield_from.check.i53.outer, label %coro_loop_wrapper_0.resume.exit60

coro_loop_wrapper_0.resume.exit60:                ; preds = %yield_from.body.i56
  %15 = getelementptr inbounds nuw i8, ptr %inner_coro.param.reload9.i55.pre.pre, i64 16
  %inner.value.i59 = load i32, ptr %15, align 4
  store i32 %inner.value.i59, ptr %1, align 4
  store i1 true, ptr %index.addr12.i, align 1
  %.pre46 = load i1, ptr %get_is_done_flag, align 1
  %.pre47 = load ptr, ptr %coro.frame.i42, align 8
  br label %coro.merge4

coro.merge4:                                      ; preds = %coro_loop_wrapper_0.resume.exit60, %coro.merge
  %inner_coro.param.reload9.i6688 = phi ptr [ %inner_coro.param.reload9.i55.pre.pre, %coro_loop_wrapper_0.resume.exit60 ], [ %inner_coro.param.reload9.i.pre.pre, %coro.merge ]
  %16 = phi ptr [ %.pre47, %coro_loop_wrapper_0.resume.exit60 ], [ %.pre45, %coro.merge ]
  %17 = phi i1 [ %.pre46, %coro_loop_wrapper_0.resume.exit60 ], [ %.pre, %coro.merge ]
  %18 = icmp eq ptr %16, null
  %19 = or i1 %17, i1 %18
  br i1 %19, label %coro.merge17, label %yield_from.check.i64.outer

yield_from.check.i64.outer:                       ; preds = %coro.merge4, %yield_from.body.i67
  %inner_coro.param.reload9.i66.pre89.ph = phi ptr [ %inner_coro.param.reload9.i66.pre.pre, %yield_from.body.i67 ], [ %inner_coro.param.reload9.i6688, %coro.merge4 ]
  %20 = load ptr, ptr %inner_coro.param.reload9.i66.pre89.ph, align 8
  %21 = icmp eq ptr %20, null
  br label %yield_from.check.i64

yield_from.check.i64:                             ; preds = %yield_from.check.i64.outer, %yield_from.check.i64
  br i1 %21, label %yield_from.check.i64, label %yield_from.body.i67

yield_from.body.i67:                              ; preds = %yield_from.check.i64
  %22 = load ptr, ptr %inner_coro.param.reload9.i66.pre89.ph, align 8
  tail call fastcc void %22(ptr nonnull %inner_coro.param.reload9.i66.pre89.ph)
  %23 = load ptr, ptr %inner_coro.param.reload9.i66.pre89.ph, align 8
  %24 = icmp eq ptr %23, null
  %inner_coro.param.reload9.i66.pre.pre = load ptr, ptr %inner_coro.param.spill.addr.i, align 8
  br i1 %24, label %yield_from.check.i64.outer, label %coro_loop_wrapper_0.resume.exit71

coro_loop_wrapper_0.resume.exit71:                ; preds = %yield_from.body.i67
  %25 = getelementptr inbounds nuw i8, ptr %inner_coro.param.reload9.i66.pre.pre, i64 16
  %inner.value.i70 = load i32, ptr %25, align 4
  store i32 %inner.value.i70, ptr %1, align 4
  store i1 true, ptr %index.addr12.i, align 1
  %.pre48 = load i1, ptr %get_is_done_flag, align 1
  %.pre49 = load ptr, ptr %coro.frame.i42, align 8
  br label %coro.merge17

coro.merge17:                                     ; preds = %coro_loop_wrapper_0.resume.exit71, %coro.merge4
  %inner_coro.param.reload9.i7793 = phi ptr [ %inner_coro.param.reload9.i66.pre.pre, %coro_loop_wrapper_0.resume.exit71 ], [ %inner_coro.param.reload9.i6688, %coro.merge4 ]
  %26 = phi ptr [ %.pre49, %coro_loop_wrapper_0.resume.exit71 ], [ %16, %coro.merge4 ]
  %27 = phi i1 [ %.pre48, %coro_loop_wrapper_0.resume.exit71 ], [ %17, %coro.merge4 ]
  %28 = icmp eq ptr %26, null
  %29 = or i1 %27, i1 %28
  br i1 %29, label %coro.merge30, label %yield_from.check.i75.outer

yield_from.check.i75.outer:                       ; preds = %coro.merge17, %yield_from.body.i78
  %inner_coro.param.reload9.i77.pre94.ph = phi ptr [ %inner_coro.param.reload9.i77.pre.pre, %yield_from.body.i78 ], [ %inner_coro.param.reload9.i7793, %coro.merge17 ]
  %30 = load ptr, ptr %inner_coro.param.reload9.i77.pre94.ph, align 8
  %31 = icmp eq ptr %30, null
  br label %yield_from.check.i75

yield_from.check.i75:                             ; preds = %yield_from.check.i75.outer, %yield_from.check.i75
  br i1 %31, label %yield_from.check.i75, label %yield_from.body.i78

yield_from.body.i78:                              ; preds = %yield_from.check.i75
  %32 = load ptr, ptr %inner_coro.param.reload9.i77.pre94.ph, align 8
  tail call fastcc void %32(ptr nonnull %inner_coro.param.reload9.i77.pre94.ph)
  %33 = load ptr, ptr %inner_coro.param.reload9.i77.pre94.ph, align 8
  %34 = icmp eq ptr %33, null
  %inner_coro.param.reload9.i77.pre.pre = load ptr, ptr %inner_coro.param.spill.addr.i, align 8
  br i1 %34, label %yield_from.check.i75.outer, label %coro_loop_wrapper_0.resume.exit82

coro_loop_wrapper_0.resume.exit82:                ; preds = %yield_from.body.i78
  %35 = getelementptr inbounds nuw i8, ptr %inner_coro.param.reload9.i77.pre.pre, i64 16
  %inner.value.i81 = load i32, ptr %35, align 4
  store i32 %inner.value.i81, ptr %1, align 4
  store i1 true, ptr %index.addr12.i, align 1
  br label %coro.merge30

coro.merge30:                                     ; preds = %entry, %coro_loop_wrapper_0.resume.exit82, %coro.merge17
  ret void
}

define noalias noundef nonnull ptr @coro_c() local_unnamed_addr #0 {
AfterCoroEnd:
  %coro.frame = tail call ptr @malloc(i32 32)
  store ptr @coro_c.resume, ptr %coro.frame, align 8
  %destroy.addr = getelementptr inbounds nuw i8, ptr %coro.frame, i64 8
  store ptr @coro_c.destroy, ptr %destroy.addr, align 8
  %0 = getelementptr inbounds nuw i8, ptr %coro.frame, i64 16
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 4 dereferenceable(9) %0, i8 0, i64 9, i1 false)
  ret ptr %coro.frame
}

declare noalias ptr @malloc(i32 %0) local_unnamed_addr #1

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr noundef captures(none) %0) local_unnamed_addr #2

define noalias noundef nonnull ptr @coro_loop_wrapper_0(ptr %inner_coro.param) local_unnamed_addr #0 {
AfterCoroSuspend:
  %coro.frame = tail call ptr @malloc(i32 40)
  store ptr @coro_loop_wrapper_0.resume, ptr %coro.frame, align 8
  %destroy.addr = getelementptr inbounds nuw i8, ptr %coro.frame, i64 8
  store ptr @coro_loop_wrapper_0.destroy, ptr %destroy.addr, align 8
  %0 = getelementptr inbounds nuw i8, ptr %coro.frame, i64 16
  store i64 0, ptr %0, align 4
  %inner_coro.param.spill.addr = getelementptr inbounds nuw i8, ptr %coro.frame, i64 24
  store ptr %inner_coro.param, ptr %inner_coro.param.spill.addr, align 8
  %index.addr12 = getelementptr inbounds nuw i8, ptr %coro.frame, i64 32
  store i1 false, ptr %index.addr12, align 1
  ret ptr %coro.frame
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite)
define internal fastcc void @coro_c.resume(ptr noundef nonnull align 8 captures(none) dereferenceable(32) %coro.handle) #3 {
resume.entry:
  %promise.reload.addr = getelementptr inbounds nuw i8, ptr %coro.handle, i64 16
  %index.addr = getelementptr inbounds nuw i8, ptr %coro.handle, i64 24
  %index = load i3, ptr %index.addr, align 8
  switch i3 %index, label %unreachable [
    i3 0, label %AfterCoroSuspend11.thread
    i3 1, label %AfterCoroSuspend14.thread
    i3 2, label %AfterCoroSuspend17.thread
    i3 3, label %AfterCoroSuspend20
  ]

CoroEnd:                                          ; preds = %AfterCoroSuspend20, %AfterCoroSuspend17.thread, %AfterCoroSuspend14.thread, %AfterCoroSuspend11.thread
  ret void

AfterCoroSuspend11.thread:                        ; preds = %resume.entry
  store i32 1, ptr %promise.reload.addr, align 8
  store i3 1, ptr %index.addr, align 8
  br label %CoroEnd

AfterCoroSuspend14.thread:                        ; preds = %resume.entry
  store i32 2, ptr %promise.reload.addr, align 8
  store i3 2, ptr %index.addr, align 8
  br label %CoroEnd

AfterCoroSuspend17.thread:                        ; preds = %resume.entry
  store i32 3, ptr %promise.reload.addr, align 8
  store i3 3, ptr %index.addr, align 8
  br label %CoroEnd

AfterCoroSuspend20:                               ; preds = %resume.entry
  store ptr null, ptr %coro.handle, align 8
  br label %CoroEnd

unreachable:                                      ; preds = %resume.entry
  unreachable
}

; Function Attrs: mustprogress nounwind willreturn memory(argmem: readwrite, inaccessiblemem: readwrite)
define internal fastcc void @coro_c.destroy(ptr noundef nonnull align 8 captures(none) dereferenceable(32) %coro.handle) #4 {
resume.entry:
  tail call void @free(ptr nonnull %coro.handle)
  ret void
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr writeonly captures(none) %0, i8 %1, i64 %2, i1 immarg %3) #5

define internal fastcc void @coro_loop_wrapper_0.resume(ptr noundef nonnull align 8 captures(none) dereferenceable(40) %coro.handle) #0 {
resume.entry:
  %inner_coro.param.reload.addr8 = getelementptr inbounds nuw i8, ptr %coro.handle, i64 24
  br label %yield_from.check

CoroEnd:                                          ; preds = %yield_from.body
  %inner_coro.param.reload.addr8.le = getelementptr inbounds nuw i8, ptr %coro.handle, i64 24
  %index.addr = getelementptr inbounds nuw i8, ptr %coro.handle, i64 32
  %promise.reload.addr = getelementptr inbounds nuw i8, ptr %coro.handle, i64 16
  %inner_coro.param.reload = load ptr, ptr %inner_coro.param.reload.addr8.le, align 8
  %0 = getelementptr inbounds nuw i8, ptr %inner_coro.param.reload, i64 16
  %inner.value = load i32, ptr %0, align 4
  store i32 %inner.value, ptr %promise.reload.addr, align 8
  store i1 true, ptr %index.addr, align 8
  ret void

yield_from.check:                                 ; preds = %yield_from.check.backedge, %resume.entry
  %inner_coro.param.reload9 = load ptr, ptr %inner_coro.param.reload.addr8, align 8
  %1 = load ptr, ptr %inner_coro.param.reload9, align 8
  %2 = icmp eq ptr %1, null
  br i1 %2, label %yield_from.check.backedge, label %yield_from.body

yield_from.check.backedge:                        ; preds = %yield_from.check, %yield_from.body
  br label %yield_from.check

yield_from.body:                                  ; preds = %yield_from.check
  %3 = load ptr, ptr %inner_coro.param.reload9, align 8
  tail call fastcc void %3(ptr nonnull %inner_coro.param.reload9)
  %4 = load ptr, ptr %inner_coro.param.reload9, align 8
  %5 = icmp eq ptr %4, null
  br i1 %5, label %yield_from.check.backedge, label %CoroEnd
}

; Function Attrs: mustprogress nounwind willreturn memory(argmem: readwrite, inaccessiblemem: readwrite)
define internal fastcc void @coro_loop_wrapper_0.destroy(ptr noundef nonnull align 8 captures(none) dereferenceable(40) %coro.handle) #4 {
resume.entry:
  tail call void @free(ptr nonnull %coro.handle)
  ret void
}

attributes #0 = { "frame-pointer"="none" }
attributes #1 = { "frame-pointer"="none" }
attributes #2 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" "frame-pointer"="none" }
attributes #3 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite) "frame-pointer"="none" }
attributes #4 = { mustprogress nounwind willreturn memory(argmem: readwrite, inaccessiblemem: readwrite) "frame-pointer"="none" }
attributes #5 = { nocallback nofree nounwind willreturn memory(argmem: write) "frame-pointer"="none" }

!0 = distinct !{!0, !1}
!1 = !{!"llvm.loop.peeled.count", i32 1}
