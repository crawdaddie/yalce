; ModuleID = 'ylc.top-level'
source_filename = "seq_cor.ylc"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"

%Seq = type { i8, [16 x i8] }

@global_storage_array = external local_unnamed_addr global [1024 x ptr]
@format_string.2 = private unnamed_addr constant [3 x i8] c"%f\00", align 1
@format_string.10 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@fmt_str.13 = private unnamed_addr constant [5 x i8] c"%.*s\00", align 1

define void @top() local_unnamed_addr #0 {
entry:
  %SEQ_LIST_TYPE_malloc = tail call ptr @malloc(i32 4)
  store i32 3, ptr %SEQ_LIST_TYPE_malloc, align 4
  store ptr %SEQ_LIST_TYPE_malloc, ptr @global_storage_array, align 8
  %SEQ_CHOOSE_TYPE_malloc = tail call ptr @malloc(i32 4)
  store i32 4, ptr %SEQ_CHOOSE_TYPE_malloc, align 4
  store ptr %SEQ_CHOOSE_TYPE_malloc, ptr getelementptr inbounds nuw (i8, ptr @global_storage_array, i64 8), align 8
  %SEQ_ALT_TYPE_malloc = tail call ptr @malloc(i32 4)
  store i32 5, ptr %SEQ_ALT_TYPE_malloc, align 4
  store ptr %SEQ_ALT_TYPE_malloc, ptr getelementptr inbounds nuw (i8, ptr @global_storage_array, i64 16), align 8
  %heap_array = tail call ptr @malloc(i32 11)
  store <8 x i8> <i8 49, i8 32, i8 99, i8 50, i8 32, i8 52, i8 32, i8 50>, ptr %heap_array, align 1
  %heap_array.repack10 = getelementptr inbounds nuw i8, ptr %heap_array, i64 8
  store i8 48, ptr %heap_array.repack10, align 1
  %heap_array.repack11 = getelementptr inbounds nuw i8, ptr %heap_array, i64 9
  store i8 48, ptr %heap_array.repack11, align 1
  %heap_array.repack12 = getelementptr inbounds nuw i8, ptr %heap_array, i64 10
  store i8 0, ptr %heap_array.repack12, align 1
  %insert_array_data = insertvalue { i32, ptr } undef, ptr %heap_array, 1
  %insert_array_size = insertvalue { i32, ptr } %insert_array_data, i32 10, 0
  %call.record_member = tail call { i8, [16 x i8] } @parse({ i32, ptr } %insert_array_size)
  %s_malloc = tail call ptr @malloc(i32 17)
  %call.record_member.elt = extractvalue { i8, [16 x i8] } %call.record_member, 0
  store i8 %call.record_member.elt, ptr %s_malloc, align 1
  %s_malloc.repack13 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 1
  %call.record_member.elt14 = extractvalue { i8, [16 x i8] } %call.record_member, 1
  %call.record_member.elt14.elt = extractvalue [16 x i8] %call.record_member.elt14, 0
  store i8 %call.record_member.elt14.elt, ptr %s_malloc.repack13, align 1
  %s_malloc.repack13.repack15 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 2
  %call.record_member.elt14.elt16 = extractvalue [16 x i8] %call.record_member.elt14, 1
  store i8 %call.record_member.elt14.elt16, ptr %s_malloc.repack13.repack15, align 1
  %s_malloc.repack13.repack17 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 3
  %call.record_member.elt14.elt18 = extractvalue [16 x i8] %call.record_member.elt14, 2
  store i8 %call.record_member.elt14.elt18, ptr %s_malloc.repack13.repack17, align 1
  %s_malloc.repack13.repack19 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 4
  %call.record_member.elt14.elt20 = extractvalue [16 x i8] %call.record_member.elt14, 3
  store i8 %call.record_member.elt14.elt20, ptr %s_malloc.repack13.repack19, align 1
  %s_malloc.repack13.repack21 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 5
  %call.record_member.elt14.elt22 = extractvalue [16 x i8] %call.record_member.elt14, 4
  store i8 %call.record_member.elt14.elt22, ptr %s_malloc.repack13.repack21, align 1
  %s_malloc.repack13.repack23 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 6
  %call.record_member.elt14.elt24 = extractvalue [16 x i8] %call.record_member.elt14, 5
  store i8 %call.record_member.elt14.elt24, ptr %s_malloc.repack13.repack23, align 1
  %s_malloc.repack13.repack25 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 7
  %call.record_member.elt14.elt26 = extractvalue [16 x i8] %call.record_member.elt14, 6
  store i8 %call.record_member.elt14.elt26, ptr %s_malloc.repack13.repack25, align 1
  %s_malloc.repack13.repack27 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 8
  %call.record_member.elt14.elt28 = extractvalue [16 x i8] %call.record_member.elt14, 7
  store i8 %call.record_member.elt14.elt28, ptr %s_malloc.repack13.repack27, align 1
  %s_malloc.repack13.repack29 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 9
  %call.record_member.elt14.elt30 = extractvalue [16 x i8] %call.record_member.elt14, 8
  store i8 %call.record_member.elt14.elt30, ptr %s_malloc.repack13.repack29, align 1
  %s_malloc.repack13.repack31 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 10
  %call.record_member.elt14.elt32 = extractvalue [16 x i8] %call.record_member.elt14, 9
  store i8 %call.record_member.elt14.elt32, ptr %s_malloc.repack13.repack31, align 1
  %s_malloc.repack13.repack33 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 11
  %call.record_member.elt14.elt34 = extractvalue [16 x i8] %call.record_member.elt14, 10
  store i8 %call.record_member.elt14.elt34, ptr %s_malloc.repack13.repack33, align 1
  %s_malloc.repack13.repack35 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 12
  %call.record_member.elt14.elt36 = extractvalue [16 x i8] %call.record_member.elt14, 11
  store i8 %call.record_member.elt14.elt36, ptr %s_malloc.repack13.repack35, align 1
  %s_malloc.repack13.repack37 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 13
  %call.record_member.elt14.elt38 = extractvalue [16 x i8] %call.record_member.elt14, 12
  store i8 %call.record_member.elt14.elt38, ptr %s_malloc.repack13.repack37, align 1
  %s_malloc.repack13.repack39 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 14
  %call.record_member.elt14.elt40 = extractvalue [16 x i8] %call.record_member.elt14, 13
  store i8 %call.record_member.elt14.elt40, ptr %s_malloc.repack13.repack39, align 1
  %s_malloc.repack13.repack41 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 15
  %call.record_member.elt14.elt42 = extractvalue [16 x i8] %call.record_member.elt14, 14
  store i8 %call.record_member.elt14.elt42, ptr %s_malloc.repack13.repack41, align 1
  %s_malloc.repack13.repack43 = getelementptr inbounds nuw i8, ptr %s_malloc, i64 16
  %call.record_member.elt14.elt44 = extractvalue [16 x i8] %call.record_member.elt14, 15
  store i8 %call.record_member.elt14.elt44, ptr %s_malloc.repack13.repack43, align 1
  store ptr %s_malloc, ptr getelementptr inbounds nuw (i8, ptr @global_storage_array, i64 24), align 8
  %call.record_member1 = tail call ptr @pat_to_coroutine({ i8, [16 x i8] } %call.record_member)
  %x_malloc = tail call ptr @malloc(i32 8)
  store ptr %call.record_member1, ptr %x_malloc, align 8
  store ptr %x_malloc, ptr getelementptr inbounds nuw (i8, ptr @global_storage_array, i64 32), align 8
  %get_is_done_flag = getelementptr inbounds nuw i8, ptr %call.record_member1, i64 33
  %0 = load i1, ptr %get_is_done_flag, align 1
  %1 = load ptr, ptr %call.record_member1, align 8
  %2 = icmp eq ptr %1, null
  %3 = or i1 %0, i1 %2
  br i1 %3, label %coro.merge, label %coro.resume

coro.resume:                                      ; preds = %entry
  %4 = load ptr, ptr %call.record_member1, align 8
  tail call fastcc void %4(ptr nonnull %call.record_member1)
  br label %coro.merge

coro.merge:                                       ; preds = %coro.resume, %entry
  ret void
}

declare noalias ptr @malloc(i32 %0) local_unnamed_addr #1

define %Seq @seq_extend(%Seq %0, %Seq %1) local_unnamed_addr #1 {
entry:
  %tag = extractvalue %Seq %0, 0
  switch i8 %tag, label %match.test.tag.3 [
    i8 0, label %match.test.0
    i8 1, label %match.test.1
    i8 2, label %match.test.2
  ]

match.test.0:                                     ; preds = %entry
  %void_ptr = load ptr, ptr @global_storage_array, align 8
  %SEQ_LIST_TYPE_load = load i32, ptr %void_ptr, align 4
  %list_el_memory_non_contiguous = tail call ptr @malloc(i32 32)
  %list_el_memory_non_contiguous2 = tail call ptr @malloc(i32 32)
  store i8 0, ptr %list_el_memory_non_contiguous, align 1
  %list_el_memory_non_contiguous.repack485 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 1
  %.elt486 = extractvalue %Seq %0, 1
  %.elt486.elt = extractvalue [16 x i8] %.elt486, 0
  store i8 %.elt486.elt, ptr %list_el_memory_non_contiguous.repack485, align 1
  %list_el_memory_non_contiguous.repack485.repack487 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 2
  %.elt486.elt488 = extractvalue [16 x i8] %.elt486, 1
  store i8 %.elt486.elt488, ptr %list_el_memory_non_contiguous.repack485.repack487, align 1
  %list_el_memory_non_contiguous.repack485.repack489 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 3
  %.elt486.elt490 = extractvalue [16 x i8] %.elt486, 2
  store i8 %.elt486.elt490, ptr %list_el_memory_non_contiguous.repack485.repack489, align 1
  %list_el_memory_non_contiguous.repack485.repack491 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 4
  %.elt486.elt492 = extractvalue [16 x i8] %.elt486, 3
  store i8 %.elt486.elt492, ptr %list_el_memory_non_contiguous.repack485.repack491, align 1
  %list_el_memory_non_contiguous.repack485.repack493 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 5
  %.elt486.elt494 = extractvalue [16 x i8] %.elt486, 4
  store i8 %.elt486.elt494, ptr %list_el_memory_non_contiguous.repack485.repack493, align 1
  %list_el_memory_non_contiguous.repack485.repack495 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 6
  %.elt486.elt496 = extractvalue [16 x i8] %.elt486, 5
  store i8 %.elt486.elt496, ptr %list_el_memory_non_contiguous.repack485.repack495, align 1
  %list_el_memory_non_contiguous.repack485.repack497 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 7
  %.elt486.elt498 = extractvalue [16 x i8] %.elt486, 6
  store i8 %.elt486.elt498, ptr %list_el_memory_non_contiguous.repack485.repack497, align 1
  %list_el_memory_non_contiguous.repack485.repack499 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 8
  %.elt486.elt500 = extractvalue [16 x i8] %.elt486, 7
  store i8 %.elt486.elt500, ptr %list_el_memory_non_contiguous.repack485.repack499, align 1
  %list_el_memory_non_contiguous.repack485.repack501 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 9
  %.elt486.elt502 = extractvalue [16 x i8] %.elt486, 8
  store i8 %.elt486.elt502, ptr %list_el_memory_non_contiguous.repack485.repack501, align 1
  %list_el_memory_non_contiguous.repack485.repack503 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 10
  %.elt486.elt504 = extractvalue [16 x i8] %.elt486, 9
  store i8 %.elt486.elt504, ptr %list_el_memory_non_contiguous.repack485.repack503, align 1
  %list_el_memory_non_contiguous.repack485.repack505 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 11
  %.elt486.elt506 = extractvalue [16 x i8] %.elt486, 10
  store i8 %.elt486.elt506, ptr %list_el_memory_non_contiguous.repack485.repack505, align 1
  %list_el_memory_non_contiguous.repack485.repack507 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 12
  %.elt486.elt508 = extractvalue [16 x i8] %.elt486, 11
  store i8 %.elt486.elt508, ptr %list_el_memory_non_contiguous.repack485.repack507, align 1
  %list_el_memory_non_contiguous.repack485.repack509 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 13
  %.elt486.elt510 = extractvalue [16 x i8] %.elt486, 12
  store i8 %.elt486.elt510, ptr %list_el_memory_non_contiguous.repack485.repack509, align 1
  %list_el_memory_non_contiguous.repack485.repack511 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 14
  %.elt486.elt512 = extractvalue [16 x i8] %.elt486, 13
  store i8 %.elt486.elt512, ptr %list_el_memory_non_contiguous.repack485.repack511, align 1
  %list_el_memory_non_contiguous.repack485.repack513 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 15
  %.elt486.elt514 = extractvalue [16 x i8] %.elt486, 14
  store i8 %.elt486.elt514, ptr %list_el_memory_non_contiguous.repack485.repack513, align 1
  %list_el_memory_non_contiguous.repack485.repack515 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 16
  %.elt486.elt516 = extractvalue [16 x i8] %.elt486, 15
  store i8 %.elt486.elt516, ptr %list_el_memory_non_contiguous.repack485.repack515, align 1
  %.elt517 = extractvalue %Seq %1, 0
  store i8 %.elt517, ptr %list_el_memory_non_contiguous2, align 1
  %list_el_memory_non_contiguous2.repack518 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 1
  %.elt519 = extractvalue %Seq %1, 1
  %.elt519.elt = extractvalue [16 x i8] %.elt519, 0
  store i8 %.elt519.elt, ptr %list_el_memory_non_contiguous2.repack518, align 1
  %list_el_memory_non_contiguous2.repack518.repack520 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 2
  %.elt519.elt521 = extractvalue [16 x i8] %.elt519, 1
  store i8 %.elt519.elt521, ptr %list_el_memory_non_contiguous2.repack518.repack520, align 1
  %list_el_memory_non_contiguous2.repack518.repack522 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 3
  %.elt519.elt523 = extractvalue [16 x i8] %.elt519, 2
  store i8 %.elt519.elt523, ptr %list_el_memory_non_contiguous2.repack518.repack522, align 1
  %list_el_memory_non_contiguous2.repack518.repack524 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 4
  %.elt519.elt525 = extractvalue [16 x i8] %.elt519, 3
  store i8 %.elt519.elt525, ptr %list_el_memory_non_contiguous2.repack518.repack524, align 1
  %list_el_memory_non_contiguous2.repack518.repack526 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 5
  %.elt519.elt527 = extractvalue [16 x i8] %.elt519, 4
  store i8 %.elt519.elt527, ptr %list_el_memory_non_contiguous2.repack518.repack526, align 1
  %list_el_memory_non_contiguous2.repack518.repack528 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 6
  %.elt519.elt529 = extractvalue [16 x i8] %.elt519, 5
  store i8 %.elt519.elt529, ptr %list_el_memory_non_contiguous2.repack518.repack528, align 1
  %list_el_memory_non_contiguous2.repack518.repack530 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 7
  %.elt519.elt531 = extractvalue [16 x i8] %.elt519, 6
  store i8 %.elt519.elt531, ptr %list_el_memory_non_contiguous2.repack518.repack530, align 1
  %list_el_memory_non_contiguous2.repack518.repack532 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 8
  %.elt519.elt533 = extractvalue [16 x i8] %.elt519, 7
  store i8 %.elt519.elt533, ptr %list_el_memory_non_contiguous2.repack518.repack532, align 1
  %list_el_memory_non_contiguous2.repack518.repack534 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 9
  %.elt519.elt535 = extractvalue [16 x i8] %.elt519, 8
  store i8 %.elt519.elt535, ptr %list_el_memory_non_contiguous2.repack518.repack534, align 1
  %list_el_memory_non_contiguous2.repack518.repack536 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 10
  %.elt519.elt537 = extractvalue [16 x i8] %.elt519, 9
  store i8 %.elt519.elt537, ptr %list_el_memory_non_contiguous2.repack518.repack536, align 1
  %list_el_memory_non_contiguous2.repack518.repack538 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 11
  %.elt519.elt539 = extractvalue [16 x i8] %.elt519, 10
  store i8 %.elt519.elt539, ptr %list_el_memory_non_contiguous2.repack518.repack538, align 1
  %list_el_memory_non_contiguous2.repack518.repack540 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 12
  %.elt519.elt541 = extractvalue [16 x i8] %.elt519, 11
  store i8 %.elt519.elt541, ptr %list_el_memory_non_contiguous2.repack518.repack540, align 1
  %list_el_memory_non_contiguous2.repack518.repack542 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 13
  %.elt519.elt543 = extractvalue [16 x i8] %.elt519, 12
  store i8 %.elt519.elt543, ptr %list_el_memory_non_contiguous2.repack518.repack542, align 1
  %list_el_memory_non_contiguous2.repack518.repack544 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 14
  %.elt519.elt545 = extractvalue [16 x i8] %.elt519, 13
  store i8 %.elt519.elt545, ptr %list_el_memory_non_contiguous2.repack518.repack544, align 1
  %list_el_memory_non_contiguous2.repack518.repack546 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 15
  %.elt519.elt547 = extractvalue [16 x i8] %.elt519, 14
  store i8 %.elt519.elt547, ptr %list_el_memory_non_contiguous2.repack518.repack546, align 1
  %list_el_memory_non_contiguous2.repack518.repack548 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 16
  %.elt519.elt549 = extractvalue [16 x i8] %.elt519, 15
  store i8 %.elt519.elt549, ptr %list_el_memory_non_contiguous2.repack518.repack548, align 1
  %next_ptr = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous, i64 24
  store ptr %list_el_memory_non_contiguous2, ptr %next_ptr, align 8
  %next_ptr4 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous2, i64 24
  store ptr null, ptr %next_ptr4, align 8
  %2 = ptrtoint ptr %list_el_memory_non_contiguous to i64
  %3 = lshr i64 %2, 56
  %4 = trunc nuw i64 %3 to i8
  %5 = lshr i64 %2, 48
  %6 = trunc i64 %5 to i8
  %7 = lshr i64 %2, 40
  %8 = trunc i64 %7 to i8
  %9 = lshr i64 %2, 32
  %10 = trunc i64 %9 to i8
  %11 = lshr i64 %2, 24
  %12 = trunc i64 %11 to i8
  %13 = lshr i64 %2, 16
  %14 = trunc i64 %13 to i8
  %15 = lshr i64 %2, 8
  %16 = trunc i64 %15 to i8
  %17 = trunc i64 %2 to i8
  %extract.t675 = trunc i32 %SEQ_LIST_TYPE_load to i8
  %extract684 = lshr i32 %SEQ_LIST_TYPE_load, 8
  %extract.t685 = trunc i32 %extract684 to i8
  %extract694 = lshr i32 %SEQ_LIST_TYPE_load, 16
  %extract.t695 = trunc i32 %extract694 to i8
  %extract704 = lshr i32 %SEQ_LIST_TYPE_load, 24
  %extract.t705 = trunc nuw i32 %extract704 to i8
  br label %common.ret

common.ret:                                       ; preds = %match.test.tag.4, %match.body.3, %match.test.2, %match.test.1, %match.test.0
  %union_to_struct64.unpack.sink670.off0 = phi i8 [ %extract.t671, %match.test.tag.4 ], [ %extract.t672, %match.body.3 ], [ %extract.t673, %match.test.2 ], [ %extract.t674, %match.test.1 ], [ %extract.t675, %match.test.0 ]
  %union_to_struct64.unpack.sink670.off8 = phi i8 [ %extract.t677, %match.test.tag.4 ], [ %extract.t679, %match.body.3 ], [ %extract.t681, %match.test.2 ], [ %extract.t683, %match.test.1 ], [ %extract.t685, %match.test.0 ]
  %union_to_struct64.unpack.sink670.off16 = phi i8 [ %extract.t687, %match.test.tag.4 ], [ %extract.t689, %match.body.3 ], [ %extract.t691, %match.test.2 ], [ %extract.t693, %match.test.1 ], [ %extract.t695, %match.test.0 ]
  %union_to_struct64.unpack.sink670.off24 = phi i8 [ %extract.t697, %match.test.tag.4 ], [ %extract.t699, %match.body.3 ], [ %extract.t701, %match.test.2 ], [ %extract.t703, %match.test.1 ], [ %extract.t705, %match.test.0 ]
  %load_as_bytes.unpack561.pn = phi i8 [ %88, %match.test.tag.4 ], [ 1, %match.body.3 ], [ 2, %match.test.2 ], [ 2, %match.test.1 ], [ 2, %match.test.0 ]
  %load_as_bytes.unpack563.pn = phi i8 [ %87, %match.test.tag.4 ], [ 0, %match.body.3 ], [ 0, %match.test.2 ], [ 0, %match.test.1 ], [ 0, %match.test.0 ]
  %load_as_bytes.unpack565.pn = phi i8 [ %85, %match.test.tag.4 ], [ 0, %match.body.3 ], [ 0, %match.test.2 ], [ 0, %match.test.1 ], [ 0, %match.test.0 ]
  %load_as_bytes.unpack567.pn = phi i8 [ %83, %match.test.tag.4 ], [ 0, %match.body.3 ], [ 0, %match.test.2 ], [ 0, %match.test.1 ], [ 0, %match.test.0 ]
  %load_as_bytes.unpack569.pn = phi i8 [ %81, %match.test.tag.4 ], [ %65, %match.body.3 ], [ %49, %match.test.2 ], [ %33, %match.test.1 ], [ %17, %match.test.0 ]
  %load_as_bytes.unpack571.pn = phi i8 [ %80, %match.test.tag.4 ], [ %64, %match.body.3 ], [ %48, %match.test.2 ], [ %32, %match.test.1 ], [ %16, %match.test.0 ]
  %load_as_bytes.unpack573.pn = phi i8 [ %78, %match.test.tag.4 ], [ %62, %match.body.3 ], [ %46, %match.test.2 ], [ %30, %match.test.1 ], [ %14, %match.test.0 ]
  %load_as_bytes.unpack575.pn = phi i8 [ %76, %match.test.tag.4 ], [ %60, %match.body.3 ], [ %44, %match.test.2 ], [ %28, %match.test.1 ], [ %12, %match.test.0 ]
  %load_as_bytes.unpack577.pn = phi i8 [ %74, %match.test.tag.4 ], [ %58, %match.body.3 ], [ %42, %match.test.2 ], [ %26, %match.test.1 ], [ %10, %match.test.0 ]
  %load_as_bytes.unpack579.pn = phi i8 [ %72, %match.test.tag.4 ], [ %56, %match.body.3 ], [ %40, %match.test.2 ], [ %24, %match.test.1 ], [ %8, %match.test.0 ]
  %load_as_bytes.unpack581.pn = phi i8 [ %70, %match.test.tag.4 ], [ %54, %match.body.3 ], [ %38, %match.test.2 ], [ %22, %match.test.1 ], [ %6, %match.test.0 ]
  %load_as_bytes.unpack583.pn = phi i8 [ %68, %match.test.tag.4 ], [ %52, %match.body.3 ], [ %36, %match.test.2 ], [ %20, %match.test.1 ], [ %4, %match.test.0 ]
  %.pn598 = insertvalue [16 x i8] poison, i8 %union_to_struct64.unpack.sink670.off0, 0
  %.pn597 = insertvalue [16 x i8] %.pn598, i8 %union_to_struct64.unpack.sink670.off8, 1
  %.pn596 = insertvalue [16 x i8] %.pn597, i8 %union_to_struct64.unpack.sink670.off16, 2
  %.pn595 = insertvalue [16 x i8] %.pn596, i8 %union_to_struct64.unpack.sink670.off24, 3
  %.pn594 = insertvalue [16 x i8] %.pn595, i8 %load_as_bytes.unpack561.pn, 4
  %.pn593 = insertvalue [16 x i8] %.pn594, i8 %load_as_bytes.unpack563.pn, 5
  %.pn592 = insertvalue [16 x i8] %.pn593, i8 %load_as_bytes.unpack565.pn, 6
  %.pn591 = insertvalue [16 x i8] %.pn592, i8 %load_as_bytes.unpack567.pn, 7
  %.pn590 = insertvalue [16 x i8] %.pn591, i8 %load_as_bytes.unpack569.pn, 8
  %.pn589 = insertvalue [16 x i8] %.pn590, i8 %load_as_bytes.unpack571.pn, 9
  %.pn588 = insertvalue [16 x i8] %.pn589, i8 %load_as_bytes.unpack573.pn, 10
  %.pn587 = insertvalue [16 x i8] %.pn588, i8 %load_as_bytes.unpack575.pn, 11
  %.pn586 = insertvalue [16 x i8] %.pn587, i8 %load_as_bytes.unpack577.pn, 12
  %.pn585 = insertvalue [16 x i8] %.pn586, i8 %load_as_bytes.unpack579.pn, 13
  %.pn = insertvalue [16 x i8] %.pn585, i8 %load_as_bytes.unpack581.pn, 14
  %load_as_bytes584.pn = insertvalue [16 x i8] %.pn, i8 %load_as_bytes.unpack583.pn, 15
  %common.ret.op = insertvalue %Seq { i8 3, [16 x i8] undef }, [16 x i8] %load_as_bytes584.pn, 1
  ret %Seq %common.ret.op

match.test.1:                                     ; preds = %entry
  %void_ptr11 = load ptr, ptr @global_storage_array, align 8
  %SEQ_LIST_TYPE_load12 = load i32, ptr %void_ptr11, align 4
  %list_el_memory_non_contiguous15 = tail call ptr @malloc(i32 32)
  %list_el_memory_non_contiguous16 = tail call ptr @malloc(i32 32)
  store i8 1, ptr %list_el_memory_non_contiguous15, align 1
  %list_el_memory_non_contiguous15.repack384 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 1
  %.elt385 = extractvalue %Seq %0, 1
  %.elt385.elt = extractvalue [16 x i8] %.elt385, 0
  store i8 %.elt385.elt, ptr %list_el_memory_non_contiguous15.repack384, align 1
  %list_el_memory_non_contiguous15.repack384.repack386 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 2
  %.elt385.elt387 = extractvalue [16 x i8] %.elt385, 1
  store i8 %.elt385.elt387, ptr %list_el_memory_non_contiguous15.repack384.repack386, align 1
  %list_el_memory_non_contiguous15.repack384.repack388 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 3
  %.elt385.elt389 = extractvalue [16 x i8] %.elt385, 2
  store i8 %.elt385.elt389, ptr %list_el_memory_non_contiguous15.repack384.repack388, align 1
  %list_el_memory_non_contiguous15.repack384.repack390 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 4
  %.elt385.elt391 = extractvalue [16 x i8] %.elt385, 3
  store i8 %.elt385.elt391, ptr %list_el_memory_non_contiguous15.repack384.repack390, align 1
  %list_el_memory_non_contiguous15.repack384.repack392 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 5
  %.elt385.elt393 = extractvalue [16 x i8] %.elt385, 4
  store i8 %.elt385.elt393, ptr %list_el_memory_non_contiguous15.repack384.repack392, align 1
  %list_el_memory_non_contiguous15.repack384.repack394 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 6
  %.elt385.elt395 = extractvalue [16 x i8] %.elt385, 5
  store i8 %.elt385.elt395, ptr %list_el_memory_non_contiguous15.repack384.repack394, align 1
  %list_el_memory_non_contiguous15.repack384.repack396 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 7
  %.elt385.elt397 = extractvalue [16 x i8] %.elt385, 6
  store i8 %.elt385.elt397, ptr %list_el_memory_non_contiguous15.repack384.repack396, align 1
  %list_el_memory_non_contiguous15.repack384.repack398 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 8
  %.elt385.elt399 = extractvalue [16 x i8] %.elt385, 7
  store i8 %.elt385.elt399, ptr %list_el_memory_non_contiguous15.repack384.repack398, align 1
  %list_el_memory_non_contiguous15.repack384.repack400 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 9
  %.elt385.elt401 = extractvalue [16 x i8] %.elt385, 8
  store i8 %.elt385.elt401, ptr %list_el_memory_non_contiguous15.repack384.repack400, align 1
  %list_el_memory_non_contiguous15.repack384.repack402 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 10
  %.elt385.elt403 = extractvalue [16 x i8] %.elt385, 9
  store i8 %.elt385.elt403, ptr %list_el_memory_non_contiguous15.repack384.repack402, align 1
  %list_el_memory_non_contiguous15.repack384.repack404 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 11
  %.elt385.elt405 = extractvalue [16 x i8] %.elt385, 10
  store i8 %.elt385.elt405, ptr %list_el_memory_non_contiguous15.repack384.repack404, align 1
  %list_el_memory_non_contiguous15.repack384.repack406 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 12
  %.elt385.elt407 = extractvalue [16 x i8] %.elt385, 11
  store i8 %.elt385.elt407, ptr %list_el_memory_non_contiguous15.repack384.repack406, align 1
  %list_el_memory_non_contiguous15.repack384.repack408 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 13
  %.elt385.elt409 = extractvalue [16 x i8] %.elt385, 12
  store i8 %.elt385.elt409, ptr %list_el_memory_non_contiguous15.repack384.repack408, align 1
  %list_el_memory_non_contiguous15.repack384.repack410 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 14
  %.elt385.elt411 = extractvalue [16 x i8] %.elt385, 13
  store i8 %.elt385.elt411, ptr %list_el_memory_non_contiguous15.repack384.repack410, align 1
  %list_el_memory_non_contiguous15.repack384.repack412 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 15
  %.elt385.elt413 = extractvalue [16 x i8] %.elt385, 14
  store i8 %.elt385.elt413, ptr %list_el_memory_non_contiguous15.repack384.repack412, align 1
  %list_el_memory_non_contiguous15.repack384.repack414 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 16
  %.elt385.elt415 = extractvalue [16 x i8] %.elt385, 15
  store i8 %.elt385.elt415, ptr %list_el_memory_non_contiguous15.repack384.repack414, align 1
  %.elt416 = extractvalue %Seq %1, 0
  store i8 %.elt416, ptr %list_el_memory_non_contiguous16, align 1
  %list_el_memory_non_contiguous16.repack417 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 1
  %.elt418 = extractvalue %Seq %1, 1
  %.elt418.elt = extractvalue [16 x i8] %.elt418, 0
  store i8 %.elt418.elt, ptr %list_el_memory_non_contiguous16.repack417, align 1
  %list_el_memory_non_contiguous16.repack417.repack419 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 2
  %.elt418.elt420 = extractvalue [16 x i8] %.elt418, 1
  store i8 %.elt418.elt420, ptr %list_el_memory_non_contiguous16.repack417.repack419, align 1
  %list_el_memory_non_contiguous16.repack417.repack421 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 3
  %.elt418.elt422 = extractvalue [16 x i8] %.elt418, 2
  store i8 %.elt418.elt422, ptr %list_el_memory_non_contiguous16.repack417.repack421, align 1
  %list_el_memory_non_contiguous16.repack417.repack423 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 4
  %.elt418.elt424 = extractvalue [16 x i8] %.elt418, 3
  store i8 %.elt418.elt424, ptr %list_el_memory_non_contiguous16.repack417.repack423, align 1
  %list_el_memory_non_contiguous16.repack417.repack425 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 5
  %.elt418.elt426 = extractvalue [16 x i8] %.elt418, 4
  store i8 %.elt418.elt426, ptr %list_el_memory_non_contiguous16.repack417.repack425, align 1
  %list_el_memory_non_contiguous16.repack417.repack427 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 6
  %.elt418.elt428 = extractvalue [16 x i8] %.elt418, 5
  store i8 %.elt418.elt428, ptr %list_el_memory_non_contiguous16.repack417.repack427, align 1
  %list_el_memory_non_contiguous16.repack417.repack429 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 7
  %.elt418.elt430 = extractvalue [16 x i8] %.elt418, 6
  store i8 %.elt418.elt430, ptr %list_el_memory_non_contiguous16.repack417.repack429, align 1
  %list_el_memory_non_contiguous16.repack417.repack431 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 8
  %.elt418.elt432 = extractvalue [16 x i8] %.elt418, 7
  store i8 %.elt418.elt432, ptr %list_el_memory_non_contiguous16.repack417.repack431, align 1
  %list_el_memory_non_contiguous16.repack417.repack433 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 9
  %.elt418.elt434 = extractvalue [16 x i8] %.elt418, 8
  store i8 %.elt418.elt434, ptr %list_el_memory_non_contiguous16.repack417.repack433, align 1
  %list_el_memory_non_contiguous16.repack417.repack435 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 10
  %.elt418.elt436 = extractvalue [16 x i8] %.elt418, 9
  store i8 %.elt418.elt436, ptr %list_el_memory_non_contiguous16.repack417.repack435, align 1
  %list_el_memory_non_contiguous16.repack417.repack437 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 11
  %.elt418.elt438 = extractvalue [16 x i8] %.elt418, 10
  store i8 %.elt418.elt438, ptr %list_el_memory_non_contiguous16.repack417.repack437, align 1
  %list_el_memory_non_contiguous16.repack417.repack439 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 12
  %.elt418.elt440 = extractvalue [16 x i8] %.elt418, 11
  store i8 %.elt418.elt440, ptr %list_el_memory_non_contiguous16.repack417.repack439, align 1
  %list_el_memory_non_contiguous16.repack417.repack441 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 13
  %.elt418.elt442 = extractvalue [16 x i8] %.elt418, 12
  store i8 %.elt418.elt442, ptr %list_el_memory_non_contiguous16.repack417.repack441, align 1
  %list_el_memory_non_contiguous16.repack417.repack443 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 14
  %.elt418.elt444 = extractvalue [16 x i8] %.elt418, 13
  store i8 %.elt418.elt444, ptr %list_el_memory_non_contiguous16.repack417.repack443, align 1
  %list_el_memory_non_contiguous16.repack417.repack445 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 15
  %.elt418.elt446 = extractvalue [16 x i8] %.elt418, 14
  store i8 %.elt418.elt446, ptr %list_el_memory_non_contiguous16.repack417.repack445, align 1
  %list_el_memory_non_contiguous16.repack417.repack447 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 16
  %.elt418.elt448 = extractvalue [16 x i8] %.elt418, 15
  store i8 %.elt418.elt448, ptr %list_el_memory_non_contiguous16.repack417.repack447, align 1
  %next_ptr19 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous15, i64 24
  store ptr %list_el_memory_non_contiguous16, ptr %next_ptr19, align 8
  %next_ptr20 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous16, i64 24
  store ptr null, ptr %next_ptr20, align 8
  %18 = ptrtoint ptr %list_el_memory_non_contiguous15 to i64
  %19 = lshr i64 %18, 56
  %20 = trunc nuw i64 %19 to i8
  %21 = lshr i64 %18, 48
  %22 = trunc i64 %21 to i8
  %23 = lshr i64 %18, 40
  %24 = trunc i64 %23 to i8
  %25 = lshr i64 %18, 32
  %26 = trunc i64 %25 to i8
  %27 = lshr i64 %18, 24
  %28 = trunc i64 %27 to i8
  %29 = lshr i64 %18, 16
  %30 = trunc i64 %29 to i8
  %31 = lshr i64 %18, 8
  %32 = trunc i64 %31 to i8
  %33 = trunc i64 %18 to i8
  %extract.t674 = trunc i32 %SEQ_LIST_TYPE_load12 to i8
  %extract682 = lshr i32 %SEQ_LIST_TYPE_load12, 8
  %extract.t683 = trunc i32 %extract682 to i8
  %extract692 = lshr i32 %SEQ_LIST_TYPE_load12, 16
  %extract.t693 = trunc i32 %extract692 to i8
  %extract702 = lshr i32 %SEQ_LIST_TYPE_load12, 24
  %extract.t703 = trunc nuw i32 %extract702 to i8
  br label %common.ret

match.test.2:                                     ; preds = %entry
  %void_ptr29 = load ptr, ptr @global_storage_array, align 8
  %SEQ_LIST_TYPE_load30 = load i32, ptr %void_ptr29, align 4
  %list_el_memory_non_contiguous33 = tail call ptr @malloc(i32 32)
  %list_el_memory_non_contiguous34 = tail call ptr @malloc(i32 32)
  store i8 2, ptr %list_el_memory_non_contiguous33, align 1
  %list_el_memory_non_contiguous33.repack283 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 1
  %.elt284 = extractvalue %Seq %0, 1
  %.elt284.elt = extractvalue [16 x i8] %.elt284, 0
  store i8 %.elt284.elt, ptr %list_el_memory_non_contiguous33.repack283, align 1
  %list_el_memory_non_contiguous33.repack283.repack285 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 2
  %.elt284.elt286 = extractvalue [16 x i8] %.elt284, 1
  store i8 %.elt284.elt286, ptr %list_el_memory_non_contiguous33.repack283.repack285, align 1
  %list_el_memory_non_contiguous33.repack283.repack287 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 3
  %.elt284.elt288 = extractvalue [16 x i8] %.elt284, 2
  store i8 %.elt284.elt288, ptr %list_el_memory_non_contiguous33.repack283.repack287, align 1
  %list_el_memory_non_contiguous33.repack283.repack289 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 4
  %.elt284.elt290 = extractvalue [16 x i8] %.elt284, 3
  store i8 %.elt284.elt290, ptr %list_el_memory_non_contiguous33.repack283.repack289, align 1
  %list_el_memory_non_contiguous33.repack283.repack291 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 5
  %.elt284.elt292 = extractvalue [16 x i8] %.elt284, 4
  store i8 %.elt284.elt292, ptr %list_el_memory_non_contiguous33.repack283.repack291, align 1
  %list_el_memory_non_contiguous33.repack283.repack293 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 6
  %.elt284.elt294 = extractvalue [16 x i8] %.elt284, 5
  store i8 %.elt284.elt294, ptr %list_el_memory_non_contiguous33.repack283.repack293, align 1
  %list_el_memory_non_contiguous33.repack283.repack295 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 7
  %.elt284.elt296 = extractvalue [16 x i8] %.elt284, 6
  store i8 %.elt284.elt296, ptr %list_el_memory_non_contiguous33.repack283.repack295, align 1
  %list_el_memory_non_contiguous33.repack283.repack297 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 8
  %.elt284.elt298 = extractvalue [16 x i8] %.elt284, 7
  store i8 %.elt284.elt298, ptr %list_el_memory_non_contiguous33.repack283.repack297, align 1
  %list_el_memory_non_contiguous33.repack283.repack299 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 9
  %.elt284.elt300 = extractvalue [16 x i8] %.elt284, 8
  store i8 %.elt284.elt300, ptr %list_el_memory_non_contiguous33.repack283.repack299, align 1
  %list_el_memory_non_contiguous33.repack283.repack301 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 10
  %.elt284.elt302 = extractvalue [16 x i8] %.elt284, 9
  store i8 %.elt284.elt302, ptr %list_el_memory_non_contiguous33.repack283.repack301, align 1
  %list_el_memory_non_contiguous33.repack283.repack303 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 11
  %.elt284.elt304 = extractvalue [16 x i8] %.elt284, 10
  store i8 %.elt284.elt304, ptr %list_el_memory_non_contiguous33.repack283.repack303, align 1
  %list_el_memory_non_contiguous33.repack283.repack305 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 12
  %.elt284.elt306 = extractvalue [16 x i8] %.elt284, 11
  store i8 %.elt284.elt306, ptr %list_el_memory_non_contiguous33.repack283.repack305, align 1
  %list_el_memory_non_contiguous33.repack283.repack307 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 13
  %.elt284.elt308 = extractvalue [16 x i8] %.elt284, 12
  store i8 %.elt284.elt308, ptr %list_el_memory_non_contiguous33.repack283.repack307, align 1
  %list_el_memory_non_contiguous33.repack283.repack309 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 14
  %.elt284.elt310 = extractvalue [16 x i8] %.elt284, 13
  store i8 %.elt284.elt310, ptr %list_el_memory_non_contiguous33.repack283.repack309, align 1
  %list_el_memory_non_contiguous33.repack283.repack311 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 15
  %.elt284.elt312 = extractvalue [16 x i8] %.elt284, 14
  store i8 %.elt284.elt312, ptr %list_el_memory_non_contiguous33.repack283.repack311, align 1
  %list_el_memory_non_contiguous33.repack283.repack313 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 16
  %.elt284.elt314 = extractvalue [16 x i8] %.elt284, 15
  store i8 %.elt284.elt314, ptr %list_el_memory_non_contiguous33.repack283.repack313, align 1
  %.elt315 = extractvalue %Seq %1, 0
  store i8 %.elt315, ptr %list_el_memory_non_contiguous34, align 1
  %list_el_memory_non_contiguous34.repack316 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 1
  %.elt317 = extractvalue %Seq %1, 1
  %.elt317.elt = extractvalue [16 x i8] %.elt317, 0
  store i8 %.elt317.elt, ptr %list_el_memory_non_contiguous34.repack316, align 1
  %list_el_memory_non_contiguous34.repack316.repack318 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 2
  %.elt317.elt319 = extractvalue [16 x i8] %.elt317, 1
  store i8 %.elt317.elt319, ptr %list_el_memory_non_contiguous34.repack316.repack318, align 1
  %list_el_memory_non_contiguous34.repack316.repack320 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 3
  %.elt317.elt321 = extractvalue [16 x i8] %.elt317, 2
  store i8 %.elt317.elt321, ptr %list_el_memory_non_contiguous34.repack316.repack320, align 1
  %list_el_memory_non_contiguous34.repack316.repack322 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 4
  %.elt317.elt323 = extractvalue [16 x i8] %.elt317, 3
  store i8 %.elt317.elt323, ptr %list_el_memory_non_contiguous34.repack316.repack322, align 1
  %list_el_memory_non_contiguous34.repack316.repack324 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 5
  %.elt317.elt325 = extractvalue [16 x i8] %.elt317, 4
  store i8 %.elt317.elt325, ptr %list_el_memory_non_contiguous34.repack316.repack324, align 1
  %list_el_memory_non_contiguous34.repack316.repack326 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 6
  %.elt317.elt327 = extractvalue [16 x i8] %.elt317, 5
  store i8 %.elt317.elt327, ptr %list_el_memory_non_contiguous34.repack316.repack326, align 1
  %list_el_memory_non_contiguous34.repack316.repack328 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 7
  %.elt317.elt329 = extractvalue [16 x i8] %.elt317, 6
  store i8 %.elt317.elt329, ptr %list_el_memory_non_contiguous34.repack316.repack328, align 1
  %list_el_memory_non_contiguous34.repack316.repack330 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 8
  %.elt317.elt331 = extractvalue [16 x i8] %.elt317, 7
  store i8 %.elt317.elt331, ptr %list_el_memory_non_contiguous34.repack316.repack330, align 1
  %list_el_memory_non_contiguous34.repack316.repack332 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 9
  %.elt317.elt333 = extractvalue [16 x i8] %.elt317, 8
  store i8 %.elt317.elt333, ptr %list_el_memory_non_contiguous34.repack316.repack332, align 1
  %list_el_memory_non_contiguous34.repack316.repack334 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 10
  %.elt317.elt335 = extractvalue [16 x i8] %.elt317, 9
  store i8 %.elt317.elt335, ptr %list_el_memory_non_contiguous34.repack316.repack334, align 1
  %list_el_memory_non_contiguous34.repack316.repack336 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 11
  %.elt317.elt337 = extractvalue [16 x i8] %.elt317, 10
  store i8 %.elt317.elt337, ptr %list_el_memory_non_contiguous34.repack316.repack336, align 1
  %list_el_memory_non_contiguous34.repack316.repack338 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 12
  %.elt317.elt339 = extractvalue [16 x i8] %.elt317, 11
  store i8 %.elt317.elt339, ptr %list_el_memory_non_contiguous34.repack316.repack338, align 1
  %list_el_memory_non_contiguous34.repack316.repack340 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 13
  %.elt317.elt341 = extractvalue [16 x i8] %.elt317, 12
  store i8 %.elt317.elt341, ptr %list_el_memory_non_contiguous34.repack316.repack340, align 1
  %list_el_memory_non_contiguous34.repack316.repack342 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 14
  %.elt317.elt343 = extractvalue [16 x i8] %.elt317, 13
  store i8 %.elt317.elt343, ptr %list_el_memory_non_contiguous34.repack316.repack342, align 1
  %list_el_memory_non_contiguous34.repack316.repack344 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 15
  %.elt317.elt345 = extractvalue [16 x i8] %.elt317, 14
  store i8 %.elt317.elt345, ptr %list_el_memory_non_contiguous34.repack316.repack344, align 1
  %list_el_memory_non_contiguous34.repack316.repack346 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 16
  %.elt317.elt347 = extractvalue [16 x i8] %.elt317, 15
  store i8 %.elt317.elt347, ptr %list_el_memory_non_contiguous34.repack316.repack346, align 1
  %next_ptr37 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous33, i64 24
  store ptr %list_el_memory_non_contiguous34, ptr %next_ptr37, align 8
  %next_ptr38 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous34, i64 24
  store ptr null, ptr %next_ptr38, align 8
  %34 = ptrtoint ptr %list_el_memory_non_contiguous33 to i64
  %35 = lshr i64 %34, 56
  %36 = trunc nuw i64 %35 to i8
  %37 = lshr i64 %34, 48
  %38 = trunc i64 %37 to i8
  %39 = lshr i64 %34, 40
  %40 = trunc i64 %39 to i8
  %41 = lshr i64 %34, 32
  %42 = trunc i64 %41 to i8
  %43 = lshr i64 %34, 24
  %44 = trunc i64 %43 to i8
  %45 = lshr i64 %34, 16
  %46 = trunc i64 %45 to i8
  %47 = lshr i64 %34, 8
  %48 = trunc i64 %47 to i8
  %49 = trunc i64 %34 to i8
  %extract.t673 = trunc i32 %SEQ_LIST_TYPE_load30 to i8
  %extract680 = lshr i32 %SEQ_LIST_TYPE_load30, 8
  %extract.t681 = trunc i32 %extract680 to i8
  %extract690 = lshr i32 %SEQ_LIST_TYPE_load30, 16
  %extract.t691 = trunc i32 %extract690 to i8
  %extract700 = lshr i32 %SEQ_LIST_TYPE_load30, 24
  %extract.t701 = trunc nuw i32 %extract700 to i8
  br label %common.ret

match.test.tag.3:                                 ; preds = %entry
  %eq_int44 = icmp eq i8 %tag, 3
  %extract_sum_type_payload45 = extractvalue %Seq %0, 1
  br i1 %eq_int44, label %match.test.3, label %match.test.tag.3.match.test.tag.4_crit_edge

match.test.tag.3.match.test.tag.4_crit_edge:      ; preds = %match.test.tag.3
  %.pre616 = extractvalue [16 x i8] %extract_sum_type_payload45, 0
  %.pre617 = extractvalue [16 x i8] %extract_sum_type_payload45, 1
  %.pre618 = extractvalue [16 x i8] %extract_sum_type_payload45, 2
  %.pre619 = extractvalue [16 x i8] %extract_sum_type_payload45, 3
  %.pre620 = extractvalue [16 x i8] %extract_sum_type_payload45, 4
  %.pre621 = extractvalue [16 x i8] %extract_sum_type_payload45, 5
  %.pre622 = extractvalue [16 x i8] %extract_sum_type_payload45, 6
  %.pre623 = extractvalue [16 x i8] %extract_sum_type_payload45, 7
  %.pre624 = extractvalue [16 x i8] %extract_sum_type_payload45, 8
  %.pre625 = extractvalue [16 x i8] %extract_sum_type_payload45, 9
  %.pre626 = extractvalue [16 x i8] %extract_sum_type_payload45, 10
  %.pre627 = extractvalue [16 x i8] %extract_sum_type_payload45, 11
  %.pre628 = extractvalue [16 x i8] %extract_sum_type_payload45, 12
  %.pre629 = extractvalue [16 x i8] %extract_sum_type_payload45, 13
  %.pre630 = extractvalue [16 x i8] %extract_sum_type_payload45, 14
  %.pre631 = extractvalue [16 x i8] %extract_sum_type_payload45, 15
  br label %match.test.tag.4

match.test.3:                                     ; preds = %match.test.tag.3
  %union_cast_temp46 = alloca [16 x i8], align 1
  %extract_sum_type_payload45.elt = extractvalue [16 x i8] %extract_sum_type_payload45, 0
  store i8 %extract_sum_type_payload45.elt, ptr %union_cast_temp46, align 1
  %union_cast_temp46.repack77 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 1
  %extract_sum_type_payload45.elt78 = extractvalue [16 x i8] %extract_sum_type_payload45, 1
  store i8 %extract_sum_type_payload45.elt78, ptr %union_cast_temp46.repack77, align 1
  %union_cast_temp46.repack79 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 2
  %extract_sum_type_payload45.elt80 = extractvalue [16 x i8] %extract_sum_type_payload45, 2
  store i8 %extract_sum_type_payload45.elt80, ptr %union_cast_temp46.repack79, align 1
  %union_cast_temp46.repack81 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 3
  %extract_sum_type_payload45.elt82 = extractvalue [16 x i8] %extract_sum_type_payload45, 3
  store i8 %extract_sum_type_payload45.elt82, ptr %union_cast_temp46.repack81, align 1
  %union_cast_temp46.repack83 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 4
  %extract_sum_type_payload45.elt84 = extractvalue [16 x i8] %extract_sum_type_payload45, 4
  store i8 %extract_sum_type_payload45.elt84, ptr %union_cast_temp46.repack83, align 1
  %union_cast_temp46.repack85 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 5
  %extract_sum_type_payload45.elt86 = extractvalue [16 x i8] %extract_sum_type_payload45, 5
  store i8 %extract_sum_type_payload45.elt86, ptr %union_cast_temp46.repack85, align 1
  %union_cast_temp46.repack87 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 6
  %extract_sum_type_payload45.elt88 = extractvalue [16 x i8] %extract_sum_type_payload45, 6
  store i8 %extract_sum_type_payload45.elt88, ptr %union_cast_temp46.repack87, align 1
  %union_cast_temp46.repack89 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 7
  %extract_sum_type_payload45.elt90 = extractvalue [16 x i8] %extract_sum_type_payload45, 7
  store i8 %extract_sum_type_payload45.elt90, ptr %union_cast_temp46.repack89, align 1
  %union_cast_temp46.repack91 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 8
  %extract_sum_type_payload45.elt92 = extractvalue [16 x i8] %extract_sum_type_payload45, 8
  store i8 %extract_sum_type_payload45.elt92, ptr %union_cast_temp46.repack91, align 1
  %union_cast_temp46.repack93 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 9
  %extract_sum_type_payload45.elt94 = extractvalue [16 x i8] %extract_sum_type_payload45, 9
  store i8 %extract_sum_type_payload45.elt94, ptr %union_cast_temp46.repack93, align 1
  %union_cast_temp46.repack95 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 10
  %extract_sum_type_payload45.elt96 = extractvalue [16 x i8] %extract_sum_type_payload45, 10
  store i8 %extract_sum_type_payload45.elt96, ptr %union_cast_temp46.repack95, align 1
  %union_cast_temp46.repack97 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 11
  %extract_sum_type_payload45.elt98 = extractvalue [16 x i8] %extract_sum_type_payload45, 11
  store i8 %extract_sum_type_payload45.elt98, ptr %union_cast_temp46.repack97, align 1
  %union_cast_temp46.repack99 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 12
  %extract_sum_type_payload45.elt100 = extractvalue [16 x i8] %extract_sum_type_payload45, 12
  store i8 %extract_sum_type_payload45.elt100, ptr %union_cast_temp46.repack99, align 1
  %union_cast_temp46.repack101 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 13
  %extract_sum_type_payload45.elt102 = extractvalue [16 x i8] %extract_sum_type_payload45, 13
  store i8 %extract_sum_type_payload45.elt102, ptr %union_cast_temp46.repack101, align 1
  %union_cast_temp46.repack103 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 14
  %extract_sum_type_payload45.elt104 = extractvalue [16 x i8] %extract_sum_type_payload45, 14
  store i8 %extract_sum_type_payload45.elt104, ptr %union_cast_temp46.repack103, align 1
  %union_cast_temp46.repack105 = getelementptr inbounds nuw i8, ptr %union_cast_temp46, i64 15
  %extract_sum_type_payload45.elt106 = extractvalue [16 x i8] %extract_sum_type_payload45, 15
  store i8 %extract_sum_type_payload45.elt106, ptr %union_cast_temp46.repack105, align 1
  %union_to_struct47.unpack108 = load i32, ptr %union_cast_temp46.repack83, align 4
  %union_to_struct47.unpack110 = load ptr, ptr %union_cast_temp46.repack91, align 8
  %eq_int48 = icmp eq i32 %union_to_struct47.unpack108, 0
  %is_null = icmp eq ptr %union_to_struct47.unpack110, null
  %match_succ_accumulator49 = and i1 %eq_int48, %is_null
  br i1 %match_succ_accumulator49, label %match.body.3, label %match.test.tag.4

match.body.3:                                     ; preds = %match.test.3
  %union_to_struct47.unpack = load i32, ptr %union_cast_temp46, align 8
  %list_el_memory_non_contiguous52 = tail call ptr @malloc(i32 32)
  %.elt214 = extractvalue %Seq %1, 0
  store i8 %.elt214, ptr %list_el_memory_non_contiguous52, align 1
  %list_el_memory_non_contiguous52.repack215 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 1
  %.elt216 = extractvalue %Seq %1, 1
  %.elt216.elt = extractvalue [16 x i8] %.elt216, 0
  store i8 %.elt216.elt, ptr %list_el_memory_non_contiguous52.repack215, align 1
  %list_el_memory_non_contiguous52.repack215.repack217 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 2
  %.elt216.elt218 = extractvalue [16 x i8] %.elt216, 1
  store i8 %.elt216.elt218, ptr %list_el_memory_non_contiguous52.repack215.repack217, align 1
  %list_el_memory_non_contiguous52.repack215.repack219 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 3
  %.elt216.elt220 = extractvalue [16 x i8] %.elt216, 2
  store i8 %.elt216.elt220, ptr %list_el_memory_non_contiguous52.repack215.repack219, align 1
  %list_el_memory_non_contiguous52.repack215.repack221 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 4
  %.elt216.elt222 = extractvalue [16 x i8] %.elt216, 3
  store i8 %.elt216.elt222, ptr %list_el_memory_non_contiguous52.repack215.repack221, align 1
  %list_el_memory_non_contiguous52.repack215.repack223 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 5
  %.elt216.elt224 = extractvalue [16 x i8] %.elt216, 4
  store i8 %.elt216.elt224, ptr %list_el_memory_non_contiguous52.repack215.repack223, align 1
  %list_el_memory_non_contiguous52.repack215.repack225 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 6
  %.elt216.elt226 = extractvalue [16 x i8] %.elt216, 5
  store i8 %.elt216.elt226, ptr %list_el_memory_non_contiguous52.repack215.repack225, align 1
  %list_el_memory_non_contiguous52.repack215.repack227 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 7
  %.elt216.elt228 = extractvalue [16 x i8] %.elt216, 6
  store i8 %.elt216.elt228, ptr %list_el_memory_non_contiguous52.repack215.repack227, align 1
  %list_el_memory_non_contiguous52.repack215.repack229 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 8
  %.elt216.elt230 = extractvalue [16 x i8] %.elt216, 7
  store i8 %.elt216.elt230, ptr %list_el_memory_non_contiguous52.repack215.repack229, align 1
  %list_el_memory_non_contiguous52.repack215.repack231 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 9
  %.elt216.elt232 = extractvalue [16 x i8] %.elt216, 8
  store i8 %.elt216.elt232, ptr %list_el_memory_non_contiguous52.repack215.repack231, align 1
  %list_el_memory_non_contiguous52.repack215.repack233 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 10
  %.elt216.elt234 = extractvalue [16 x i8] %.elt216, 9
  store i8 %.elt216.elt234, ptr %list_el_memory_non_contiguous52.repack215.repack233, align 1
  %list_el_memory_non_contiguous52.repack215.repack235 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 11
  %.elt216.elt236 = extractvalue [16 x i8] %.elt216, 10
  store i8 %.elt216.elt236, ptr %list_el_memory_non_contiguous52.repack215.repack235, align 1
  %list_el_memory_non_contiguous52.repack215.repack237 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 12
  %.elt216.elt238 = extractvalue [16 x i8] %.elt216, 11
  store i8 %.elt216.elt238, ptr %list_el_memory_non_contiguous52.repack215.repack237, align 1
  %list_el_memory_non_contiguous52.repack215.repack239 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 13
  %.elt216.elt240 = extractvalue [16 x i8] %.elt216, 12
  store i8 %.elt216.elt240, ptr %list_el_memory_non_contiguous52.repack215.repack239, align 1
  %list_el_memory_non_contiguous52.repack215.repack241 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 14
  %.elt216.elt242 = extractvalue [16 x i8] %.elt216, 13
  store i8 %.elt216.elt242, ptr %list_el_memory_non_contiguous52.repack215.repack241, align 1
  %list_el_memory_non_contiguous52.repack215.repack243 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 15
  %.elt216.elt244 = extractvalue [16 x i8] %.elt216, 14
  store i8 %.elt216.elt244, ptr %list_el_memory_non_contiguous52.repack215.repack243, align 1
  %list_el_memory_non_contiguous52.repack215.repack245 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 16
  %.elt216.elt246 = extractvalue [16 x i8] %.elt216, 15
  store i8 %.elt216.elt246, ptr %list_el_memory_non_contiguous52.repack215.repack245, align 1
  %next_ptr54 = getelementptr inbounds nuw i8, ptr %list_el_memory_non_contiguous52, i64 24
  store ptr null, ptr %next_ptr54, align 8
  %50 = ptrtoint ptr %list_el_memory_non_contiguous52 to i64
  %51 = lshr i64 %50, 56
  %52 = trunc nuw i64 %51 to i8
  %53 = lshr i64 %50, 48
  %54 = trunc i64 %53 to i8
  %55 = lshr i64 %50, 40
  %56 = trunc i64 %55 to i8
  %57 = lshr i64 %50, 32
  %58 = trunc i64 %57 to i8
  %59 = lshr i64 %50, 24
  %60 = trunc i64 %59 to i8
  %61 = lshr i64 %50, 16
  %62 = trunc i64 %61 to i8
  %63 = lshr i64 %50, 8
  %64 = trunc i64 %63 to i8
  %65 = trunc i64 %50 to i8
  %extract.t672 = trunc i32 %union_to_struct47.unpack to i8
  %extract678 = lshr i32 %union_to_struct47.unpack, 8
  %extract.t679 = trunc i32 %extract678 to i8
  %extract688 = lshr i32 %union_to_struct47.unpack, 16
  %extract.t689 = trunc i32 %extract688 to i8
  %extract698 = lshr i32 %union_to_struct47.unpack, 24
  %extract.t699 = trunc nuw i32 %extract698 to i8
  br label %common.ret

match.test.tag.4:                                 ; preds = %match.test.tag.3.match.test.tag.4_crit_edge, %match.test.3
  %extract_sum_type_payload62.elt141.pre-phi = phi i8 [ %.pre631, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt106, %match.test.3 ]
  %extract_sum_type_payload62.elt139.pre-phi = phi i8 [ %.pre630, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt104, %match.test.3 ]
  %extract_sum_type_payload62.elt137.pre-phi = phi i8 [ %.pre629, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt102, %match.test.3 ]
  %extract_sum_type_payload62.elt135.pre-phi = phi i8 [ %.pre628, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt100, %match.test.3 ]
  %extract_sum_type_payload62.elt133.pre-phi = phi i8 [ %.pre627, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt98, %match.test.3 ]
  %extract_sum_type_payload62.elt131.pre-phi = phi i8 [ %.pre626, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt96, %match.test.3 ]
  %extract_sum_type_payload62.elt129.pre-phi = phi i8 [ %.pre625, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt94, %match.test.3 ]
  %extract_sum_type_payload62.elt127.pre-phi = phi i8 [ %.pre624, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt92, %match.test.3 ]
  %extract_sum_type_payload62.elt125.pre-phi = phi i8 [ %.pre623, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt90, %match.test.3 ]
  %extract_sum_type_payload62.elt123.pre-phi = phi i8 [ %.pre622, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt88, %match.test.3 ]
  %extract_sum_type_payload62.elt121.pre-phi = phi i8 [ %.pre621, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt86, %match.test.3 ]
  %extract_sum_type_payload62.elt119.pre-phi = phi i8 [ %.pre620, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt84, %match.test.3 ]
  %extract_sum_type_payload62.elt117.pre-phi = phi i8 [ %.pre619, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt82, %match.test.3 ]
  %extract_sum_type_payload62.elt115.pre-phi = phi i8 [ %.pre618, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt80, %match.test.3 ]
  %extract_sum_type_payload62.elt113.pre-phi = phi i8 [ %.pre617, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt78, %match.test.3 ]
  %extract_sum_type_payload62.elt.pre-phi = phi i8 [ %.pre616, %match.test.tag.3.match.test.tag.4_crit_edge ], [ %extract_sum_type_payload45.elt, %match.test.3 ]
  tail call void @llvm.assume(i1 %eq_int44)
  %union_cast_temp63 = alloca [16 x i8], align 1
  store i8 %extract_sum_type_payload62.elt.pre-phi, ptr %union_cast_temp63, align 1
  %union_cast_temp63.repack112 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 1
  store i8 %extract_sum_type_payload62.elt113.pre-phi, ptr %union_cast_temp63.repack112, align 1
  %union_cast_temp63.repack114 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 2
  store i8 %extract_sum_type_payload62.elt115.pre-phi, ptr %union_cast_temp63.repack114, align 1
  %union_cast_temp63.repack116 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 3
  store i8 %extract_sum_type_payload62.elt117.pre-phi, ptr %union_cast_temp63.repack116, align 1
  %union_cast_temp63.repack118 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 4
  store i8 %extract_sum_type_payload62.elt119.pre-phi, ptr %union_cast_temp63.repack118, align 1
  %union_cast_temp63.repack120 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 5
  store i8 %extract_sum_type_payload62.elt121.pre-phi, ptr %union_cast_temp63.repack120, align 1
  %union_cast_temp63.repack122 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 6
  store i8 %extract_sum_type_payload62.elt123.pre-phi, ptr %union_cast_temp63.repack122, align 1
  %union_cast_temp63.repack124 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 7
  store i8 %extract_sum_type_payload62.elt125.pre-phi, ptr %union_cast_temp63.repack124, align 1
  %union_cast_temp63.repack126 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 8
  store i8 %extract_sum_type_payload62.elt127.pre-phi, ptr %union_cast_temp63.repack126, align 1
  %union_cast_temp63.repack128 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 9
  store i8 %extract_sum_type_payload62.elt129.pre-phi, ptr %union_cast_temp63.repack128, align 1
  %union_cast_temp63.repack130 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 10
  store i8 %extract_sum_type_payload62.elt131.pre-phi, ptr %union_cast_temp63.repack130, align 1
  %union_cast_temp63.repack132 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 11
  store i8 %extract_sum_type_payload62.elt133.pre-phi, ptr %union_cast_temp63.repack132, align 1
  %union_cast_temp63.repack134 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 12
  store i8 %extract_sum_type_payload62.elt135.pre-phi, ptr %union_cast_temp63.repack134, align 1
  %union_cast_temp63.repack136 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 13
  store i8 %extract_sum_type_payload62.elt137.pre-phi, ptr %union_cast_temp63.repack136, align 1
  %union_cast_temp63.repack138 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 14
  store i8 %extract_sum_type_payload62.elt139.pre-phi, ptr %union_cast_temp63.repack138, align 1
  %union_cast_temp63.repack140 = getelementptr inbounds nuw i8, ptr %union_cast_temp63, i64 15
  store i8 %extract_sum_type_payload62.elt141.pre-phi, ptr %union_cast_temp63.repack140, align 1
  %union_to_struct64.unpack = load i32, ptr %union_cast_temp63, align 8
  %union_to_struct64.unpack143 = load i32, ptr %union_cast_temp63.repack118, align 4
  %union_to_struct64.unpack145 = load ptr, ptr %union_cast_temp63.repack126, align 8
  %"+_int" = add i32 %union_to_struct64.unpack143, 1
  %new_node = tail call ptr @malloc(i32 32)
  %.elt = extractvalue %Seq %1, 0
  store i8 %.elt, ptr %new_node, align 1
  %new_node.repack147 = getelementptr inbounds nuw i8, ptr %new_node, i64 1
  %.elt148 = extractvalue %Seq %1, 1
  %.elt148.elt = extractvalue [16 x i8] %.elt148, 0
  store i8 %.elt148.elt, ptr %new_node.repack147, align 1
  %new_node.repack147.repack149 = getelementptr inbounds nuw i8, ptr %new_node, i64 2
  %.elt148.elt150 = extractvalue [16 x i8] %.elt148, 1
  store i8 %.elt148.elt150, ptr %new_node.repack147.repack149, align 1
  %new_node.repack147.repack151 = getelementptr inbounds nuw i8, ptr %new_node, i64 3
  %.elt148.elt152 = extractvalue [16 x i8] %.elt148, 2
  store i8 %.elt148.elt152, ptr %new_node.repack147.repack151, align 1
  %new_node.repack147.repack153 = getelementptr inbounds nuw i8, ptr %new_node, i64 4
  %.elt148.elt154 = extractvalue [16 x i8] %.elt148, 3
  store i8 %.elt148.elt154, ptr %new_node.repack147.repack153, align 1
  %new_node.repack147.repack155 = getelementptr inbounds nuw i8, ptr %new_node, i64 5
  %.elt148.elt156 = extractvalue [16 x i8] %.elt148, 4
  store i8 %.elt148.elt156, ptr %new_node.repack147.repack155, align 1
  %new_node.repack147.repack157 = getelementptr inbounds nuw i8, ptr %new_node, i64 6
  %.elt148.elt158 = extractvalue [16 x i8] %.elt148, 5
  store i8 %.elt148.elt158, ptr %new_node.repack147.repack157, align 1
  %new_node.repack147.repack159 = getelementptr inbounds nuw i8, ptr %new_node, i64 7
  %.elt148.elt160 = extractvalue [16 x i8] %.elt148, 6
  store i8 %.elt148.elt160, ptr %new_node.repack147.repack159, align 1
  %new_node.repack147.repack161 = getelementptr inbounds nuw i8, ptr %new_node, i64 8
  %.elt148.elt162 = extractvalue [16 x i8] %.elt148, 7
  store i8 %.elt148.elt162, ptr %new_node.repack147.repack161, align 1
  %new_node.repack147.repack163 = getelementptr inbounds nuw i8, ptr %new_node, i64 9
  %.elt148.elt164 = extractvalue [16 x i8] %.elt148, 8
  store i8 %.elt148.elt164, ptr %new_node.repack147.repack163, align 1
  %new_node.repack147.repack165 = getelementptr inbounds nuw i8, ptr %new_node, i64 10
  %.elt148.elt166 = extractvalue [16 x i8] %.elt148, 9
  store i8 %.elt148.elt166, ptr %new_node.repack147.repack165, align 1
  %new_node.repack147.repack167 = getelementptr inbounds nuw i8, ptr %new_node, i64 11
  %.elt148.elt168 = extractvalue [16 x i8] %.elt148, 10
  store i8 %.elt148.elt168, ptr %new_node.repack147.repack167, align 1
  %new_node.repack147.repack169 = getelementptr inbounds nuw i8, ptr %new_node, i64 12
  %.elt148.elt170 = extractvalue [16 x i8] %.elt148, 11
  store i8 %.elt148.elt170, ptr %new_node.repack147.repack169, align 1
  %new_node.repack147.repack171 = getelementptr inbounds nuw i8, ptr %new_node, i64 13
  %.elt148.elt172 = extractvalue [16 x i8] %.elt148, 12
  store i8 %.elt148.elt172, ptr %new_node.repack147.repack171, align 1
  %new_node.repack147.repack173 = getelementptr inbounds nuw i8, ptr %new_node, i64 14
  %.elt148.elt174 = extractvalue [16 x i8] %.elt148, 13
  store i8 %.elt148.elt174, ptr %new_node.repack147.repack173, align 1
  %new_node.repack147.repack175 = getelementptr inbounds nuw i8, ptr %new_node, i64 15
  %.elt148.elt176 = extractvalue [16 x i8] %.elt148, 14
  store i8 %.elt148.elt176, ptr %new_node.repack147.repack175, align 1
  %new_node.repack147.repack177 = getelementptr inbounds nuw i8, ptr %new_node, i64 16
  %.elt148.elt178 = extractvalue [16 x i8] %.elt148, 15
  store i8 %.elt148.elt178, ptr %new_node.repack147.repack177, align 1
  %next_ptr71 = getelementptr inbounds nuw i8, ptr %new_node, i64 24
  store ptr %union_to_struct64.unpack145, ptr %next_ptr71, align 8
  %66 = ptrtoint ptr %new_node to i64
  %67 = lshr i64 %66, 56
  %68 = trunc nuw i64 %67 to i8
  %69 = lshr i64 %66, 48
  %70 = trunc i64 %69 to i8
  %71 = lshr i64 %66, 40
  %72 = trunc i64 %71 to i8
  %73 = lshr i64 %66, 32
  %74 = trunc i64 %73 to i8
  %75 = lshr i64 %66, 24
  %76 = trunc i64 %75 to i8
  %77 = lshr i64 %66, 16
  %78 = trunc i64 %77 to i8
  %79 = lshr i64 %66, 8
  %80 = trunc i64 %79 to i8
  %81 = trunc i64 %66 to i8
  %82 = lshr i32 %"+_int", 24
  %83 = trunc nuw i32 %82 to i8
  %84 = lshr i32 %"+_int", 16
  %85 = trunc i32 %84 to i8
  %86 = lshr i32 %"+_int", 8
  %87 = trunc i32 %86 to i8
  %88 = trunc i32 %"+_int" to i8
  %extract.t671 = trunc i32 %union_to_struct64.unpack to i8
  %extract676 = lshr i32 %union_to_struct64.unpack, 8
  %extract.t677 = trunc i32 %extract676 to i8
  %extract686 = lshr i32 %union_to_struct64.unpack, 16
  %extract.t687 = trunc i32 %extract686 to i8
  %extract696 = lshr i32 %union_to_struct64.unpack, 24
  %extract.t697 = trunc nuw i32 %extract696 to i8
  br label %common.ret
}

define %Seq @seq_reverse(%Seq %0) local_unnamed_addr #1 {
entry:
  %tag = extractvalue %Seq %0, 0
  %switch = icmp ult i8 %tag, 3
  br i1 %switch, label %common.ret, label %match.test.tag.3

common.ret:                                       ; preds = %entry, %match.test.tag.3
  %common.ret.op = phi %Seq [ %"insert variant data", %match.test.tag.3 ], [ %0, %entry ]
  ret %Seq %common.ret.op

match.test.tag.3:                                 ; preds = %entry
  %eq_int11 = icmp eq i8 %tag, 3
  tail call void @llvm.assume(i1 %eq_int11)
  %extract_sum_type_payload13 = extractvalue %Seq %0, 1
  %union_cast_temp14 = alloca [16 x i8], align 1
  %extract_sum_type_payload13.elt = extractvalue [16 x i8] %extract_sum_type_payload13, 0
  store i8 %extract_sum_type_payload13.elt, ptr %union_cast_temp14, align 1
  %union_cast_temp14.repack18 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 1
  %extract_sum_type_payload13.elt19 = extractvalue [16 x i8] %extract_sum_type_payload13, 1
  store i8 %extract_sum_type_payload13.elt19, ptr %union_cast_temp14.repack18, align 1
  %union_cast_temp14.repack20 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 2
  %extract_sum_type_payload13.elt21 = extractvalue [16 x i8] %extract_sum_type_payload13, 2
  store i8 %extract_sum_type_payload13.elt21, ptr %union_cast_temp14.repack20, align 1
  %union_cast_temp14.repack22 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 3
  %extract_sum_type_payload13.elt23 = extractvalue [16 x i8] %extract_sum_type_payload13, 3
  store i8 %extract_sum_type_payload13.elt23, ptr %union_cast_temp14.repack22, align 1
  %union_cast_temp14.repack24 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 4
  %extract_sum_type_payload13.elt25 = extractvalue [16 x i8] %extract_sum_type_payload13, 4
  store i8 %extract_sum_type_payload13.elt25, ptr %union_cast_temp14.repack24, align 1
  %union_cast_temp14.repack26 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 5
  %extract_sum_type_payload13.elt27 = extractvalue [16 x i8] %extract_sum_type_payload13, 5
  store i8 %extract_sum_type_payload13.elt27, ptr %union_cast_temp14.repack26, align 1
  %union_cast_temp14.repack28 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 6
  %extract_sum_type_payload13.elt29 = extractvalue [16 x i8] %extract_sum_type_payload13, 6
  store i8 %extract_sum_type_payload13.elt29, ptr %union_cast_temp14.repack28, align 1
  %union_cast_temp14.repack30 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 7
  %extract_sum_type_payload13.elt31 = extractvalue [16 x i8] %extract_sum_type_payload13, 7
  store i8 %extract_sum_type_payload13.elt31, ptr %union_cast_temp14.repack30, align 1
  %union_cast_temp14.repack32 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 8
  %extract_sum_type_payload13.elt33 = extractvalue [16 x i8] %extract_sum_type_payload13, 8
  store i8 %extract_sum_type_payload13.elt33, ptr %union_cast_temp14.repack32, align 1
  %union_cast_temp14.repack34 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 9
  %extract_sum_type_payload13.elt35 = extractvalue [16 x i8] %extract_sum_type_payload13, 9
  store i8 %extract_sum_type_payload13.elt35, ptr %union_cast_temp14.repack34, align 1
  %union_cast_temp14.repack36 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 10
  %extract_sum_type_payload13.elt37 = extractvalue [16 x i8] %extract_sum_type_payload13, 10
  store i8 %extract_sum_type_payload13.elt37, ptr %union_cast_temp14.repack36, align 1
  %union_cast_temp14.repack38 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 11
  %extract_sum_type_payload13.elt39 = extractvalue [16 x i8] %extract_sum_type_payload13, 11
  store i8 %extract_sum_type_payload13.elt39, ptr %union_cast_temp14.repack38, align 1
  %union_cast_temp14.repack40 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 12
  %extract_sum_type_payload13.elt41 = extractvalue [16 x i8] %extract_sum_type_payload13, 12
  store i8 %extract_sum_type_payload13.elt41, ptr %union_cast_temp14.repack40, align 1
  %union_cast_temp14.repack42 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 13
  %extract_sum_type_payload13.elt43 = extractvalue [16 x i8] %extract_sum_type_payload13, 13
  store i8 %extract_sum_type_payload13.elt43, ptr %union_cast_temp14.repack42, align 1
  %union_cast_temp14.repack44 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 14
  %extract_sum_type_payload13.elt45 = extractvalue [16 x i8] %extract_sum_type_payload13, 14
  store i8 %extract_sum_type_payload13.elt45, ptr %union_cast_temp14.repack44, align 1
  %union_cast_temp14.repack46 = getelementptr inbounds nuw i8, ptr %union_cast_temp14, i64 15
  %extract_sum_type_payload13.elt47 = extractvalue [16 x i8] %extract_sum_type_payload13, 15
  store i8 %extract_sum_type_payload13.elt47, ptr %union_cast_temp14.repack46, align 1
  %union_to_struct15.unpack = load i32, ptr %union_cast_temp14, align 8
  %union_to_struct15.unpack49 = load i32, ptr %union_cast_temp14.repack24, align 4
  %union_to_struct15.unpack51 = load ptr, ptr %union_cast_temp14.repack32, align 8
  %call.seq_list_rev = tail call ptr @seq_list_rev(ptr %union_to_struct15.unpack51, ptr null)
  %1 = trunc i32 %union_to_struct15.unpack to i8
  %2 = insertvalue [16 x i8] poison, i8 %1, 0
  %3 = lshr i32 %union_to_struct15.unpack, 8
  %4 = trunc i32 %3 to i8
  %5 = insertvalue [16 x i8] %2, i8 %4, 1
  %6 = lshr i32 %union_to_struct15.unpack, 16
  %7 = trunc i32 %6 to i8
  %8 = insertvalue [16 x i8] %5, i8 %7, 2
  %9 = lshr i32 %union_to_struct15.unpack, 24
  %10 = trunc nuw i32 %9 to i8
  %11 = insertvalue [16 x i8] %8, i8 %10, 3
  %12 = trunc i32 %union_to_struct15.unpack49 to i8
  %13 = insertvalue [16 x i8] %11, i8 %12, 4
  %14 = lshr i32 %union_to_struct15.unpack49, 8
  %15 = trunc i32 %14 to i8
  %16 = insertvalue [16 x i8] %13, i8 %15, 5
  %17 = lshr i32 %union_to_struct15.unpack49, 16
  %18 = trunc i32 %17 to i8
  %19 = insertvalue [16 x i8] %16, i8 %18, 6
  %20 = lshr i32 %union_to_struct15.unpack49, 24
  %21 = trunc nuw i32 %20 to i8
  %22 = insertvalue [16 x i8] %19, i8 %21, 7
  %23 = ptrtoint ptr %call.seq_list_rev to i64
  %24 = trunc i64 %23 to i8
  %25 = insertvalue [16 x i8] %22, i8 %24, 8
  %26 = lshr i64 %23, 8
  %27 = trunc i64 %26 to i8
  %28 = insertvalue [16 x i8] %25, i8 %27, 9
  %29 = lshr i64 %23, 16
  %30 = trunc i64 %29 to i8
  %31 = insertvalue [16 x i8] %28, i8 %30, 10
  %32 = lshr i64 %23, 24
  %33 = trunc i64 %32 to i8
  %34 = insertvalue [16 x i8] %31, i8 %33, 11
  %35 = lshr i64 %23, 32
  %36 = trunc i64 %35 to i8
  %37 = insertvalue [16 x i8] %34, i8 %36, 12
  %38 = lshr i64 %23, 40
  %39 = trunc i64 %38 to i8
  %40 = insertvalue [16 x i8] %37, i8 %39, 13
  %41 = lshr i64 %23, 48
  %42 = trunc i64 %41 to i8
  %43 = insertvalue [16 x i8] %40, i8 %42, 14
  %44 = lshr i64 %23, 56
  %45 = trunc nuw i64 %44 to i8
  %load_as_bytes87 = insertvalue [16 x i8] %43, i8 %45, 15
  %"insert variant data" = insertvalue %Seq { i8 3, [16 x i8] undef }, [16 x i8] %load_as_bytes87, 1
  br label %common.ret
}

define ptr @seq_list_rev(ptr readonly captures(address_is_null) %0, ptr %1) local_unnamed_addr #0 {
entry:
  %is_null71 = icmp eq ptr %0, null
  br i1 %is_null71, label %common.ret, label %list_cons_test_elements

common.ret:                                       ; preds = %list_cons_test_elements, %entry
  %.tr70.lcssa = phi ptr [ %1, %entry ], [ %new_node, %list_cons_test_elements ]
  ret ptr %.tr70.lcssa

list_cons_test_elements:                          ; preds = %entry, %list_cons_test_elements
  %.tr7073 = phi ptr [ %new_node, %list_cons_test_elements ], [ %1, %entry ]
  %.tr72 = phi ptr [ %list_rest, %list_cons_test_elements ], [ %0, %entry ]
  %list_head.unpack = load i8, ptr %.tr72, align 1
  %2 = insertvalue %Seq poison, i8 %list_head.unpack, 0
  %list_head.elt4 = getelementptr inbounds nuw i8, ptr %.tr72, i64 1
  %list_head.unpack5.unpack = load i8, ptr %list_head.elt4, align 1
  %3 = insertvalue [16 x i8] poison, i8 %list_head.unpack5.unpack, 0
  %list_head.unpack5.elt7 = getelementptr inbounds nuw i8, ptr %.tr72, i64 2
  %list_head.unpack5.unpack8 = load i8, ptr %list_head.unpack5.elt7, align 1
  %4 = insertvalue [16 x i8] %3, i8 %list_head.unpack5.unpack8, 1
  %list_head.unpack5.elt9 = getelementptr inbounds nuw i8, ptr %.tr72, i64 3
  %list_head.unpack5.unpack10 = load i8, ptr %list_head.unpack5.elt9, align 1
  %5 = insertvalue [16 x i8] %4, i8 %list_head.unpack5.unpack10, 2
  %list_head.unpack5.elt11 = getelementptr inbounds nuw i8, ptr %.tr72, i64 4
  %list_head.unpack5.unpack12 = load i8, ptr %list_head.unpack5.elt11, align 1
  %6 = insertvalue [16 x i8] %5, i8 %list_head.unpack5.unpack12, 3
  %list_head.unpack5.elt13 = getelementptr inbounds nuw i8, ptr %.tr72, i64 5
  %list_head.unpack5.unpack14 = load i8, ptr %list_head.unpack5.elt13, align 1
  %7 = insertvalue [16 x i8] %6, i8 %list_head.unpack5.unpack14, 4
  %list_head.unpack5.elt15 = getelementptr inbounds nuw i8, ptr %.tr72, i64 6
  %list_head.unpack5.unpack16 = load i8, ptr %list_head.unpack5.elt15, align 1
  %8 = insertvalue [16 x i8] %7, i8 %list_head.unpack5.unpack16, 5
  %list_head.unpack5.elt17 = getelementptr inbounds nuw i8, ptr %.tr72, i64 7
  %list_head.unpack5.unpack18 = load i8, ptr %list_head.unpack5.elt17, align 1
  %9 = insertvalue [16 x i8] %8, i8 %list_head.unpack5.unpack18, 6
  %list_head.unpack5.elt19 = getelementptr inbounds nuw i8, ptr %.tr72, i64 8
  %list_head.unpack5.unpack20 = load i8, ptr %list_head.unpack5.elt19, align 1
  %10 = insertvalue [16 x i8] %9, i8 %list_head.unpack5.unpack20, 7
  %list_head.unpack5.elt21 = getelementptr inbounds nuw i8, ptr %.tr72, i64 9
  %list_head.unpack5.unpack22 = load i8, ptr %list_head.unpack5.elt21, align 1
  %11 = insertvalue [16 x i8] %10, i8 %list_head.unpack5.unpack22, 8
  %list_head.unpack5.elt23 = getelementptr inbounds nuw i8, ptr %.tr72, i64 10
  %list_head.unpack5.unpack24 = load i8, ptr %list_head.unpack5.elt23, align 1
  %12 = insertvalue [16 x i8] %11, i8 %list_head.unpack5.unpack24, 9
  %list_head.unpack5.elt25 = getelementptr inbounds nuw i8, ptr %.tr72, i64 11
  %list_head.unpack5.unpack26 = load i8, ptr %list_head.unpack5.elt25, align 1
  %13 = insertvalue [16 x i8] %12, i8 %list_head.unpack5.unpack26, 10
  %list_head.unpack5.elt27 = getelementptr inbounds nuw i8, ptr %.tr72, i64 12
  %list_head.unpack5.unpack28 = load i8, ptr %list_head.unpack5.elt27, align 1
  %14 = insertvalue [16 x i8] %13, i8 %list_head.unpack5.unpack28, 11
  %list_head.unpack5.elt29 = getelementptr inbounds nuw i8, ptr %.tr72, i64 13
  %list_head.unpack5.unpack30 = load i8, ptr %list_head.unpack5.elt29, align 1
  %15 = insertvalue [16 x i8] %14, i8 %list_head.unpack5.unpack30, 12
  %list_head.unpack5.elt31 = getelementptr inbounds nuw i8, ptr %.tr72, i64 14
  %list_head.unpack5.unpack32 = load i8, ptr %list_head.unpack5.elt31, align 1
  %16 = insertvalue [16 x i8] %15, i8 %list_head.unpack5.unpack32, 13
  %list_head.unpack5.elt33 = getelementptr inbounds nuw i8, ptr %.tr72, i64 15
  %list_head.unpack5.unpack34 = load i8, ptr %list_head.unpack5.elt33, align 1
  %17 = insertvalue [16 x i8] %16, i8 %list_head.unpack5.unpack34, 14
  %list_head.unpack5.elt35 = getelementptr inbounds nuw i8, ptr %.tr72, i64 16
  %list_head.unpack5.unpack36 = load i8, ptr %list_head.unpack5.elt35, align 1
  %list_head.unpack537 = insertvalue [16 x i8] %17, i8 %list_head.unpack5.unpack36, 15
  %list_head6 = insertvalue %Seq %2, [16 x i8] %list_head.unpack537, 1
  %item_ptr2 = getelementptr inbounds nuw i8, ptr %.tr72, i64 24
  %list_rest = load ptr, ptr %item_ptr2, align 8
  %call.seq_reverse = tail call %Seq @seq_reverse(%Seq %list_head6)
  %new_node = tail call ptr @malloc(i32 32)
  %call.seq_reverse.elt = extractvalue %Seq %call.seq_reverse, 0
  store i8 %call.seq_reverse.elt, ptr %new_node, align 1
  %new_node.repack38 = getelementptr inbounds nuw i8, ptr %new_node, i64 1
  %call.seq_reverse.elt39 = extractvalue %Seq %call.seq_reverse, 1
  %call.seq_reverse.elt39.elt = extractvalue [16 x i8] %call.seq_reverse.elt39, 0
  store i8 %call.seq_reverse.elt39.elt, ptr %new_node.repack38, align 1
  %new_node.repack38.repack40 = getelementptr inbounds nuw i8, ptr %new_node, i64 2
  %call.seq_reverse.elt39.elt41 = extractvalue [16 x i8] %call.seq_reverse.elt39, 1
  store i8 %call.seq_reverse.elt39.elt41, ptr %new_node.repack38.repack40, align 1
  %new_node.repack38.repack42 = getelementptr inbounds nuw i8, ptr %new_node, i64 3
  %call.seq_reverse.elt39.elt43 = extractvalue [16 x i8] %call.seq_reverse.elt39, 2
  store i8 %call.seq_reverse.elt39.elt43, ptr %new_node.repack38.repack42, align 1
  %new_node.repack38.repack44 = getelementptr inbounds nuw i8, ptr %new_node, i64 4
  %call.seq_reverse.elt39.elt45 = extractvalue [16 x i8] %call.seq_reverse.elt39, 3
  store i8 %call.seq_reverse.elt39.elt45, ptr %new_node.repack38.repack44, align 1
  %new_node.repack38.repack46 = getelementptr inbounds nuw i8, ptr %new_node, i64 5
  %call.seq_reverse.elt39.elt47 = extractvalue [16 x i8] %call.seq_reverse.elt39, 4
  store i8 %call.seq_reverse.elt39.elt47, ptr %new_node.repack38.repack46, align 1
  %new_node.repack38.repack48 = getelementptr inbounds nuw i8, ptr %new_node, i64 6
  %call.seq_reverse.elt39.elt49 = extractvalue [16 x i8] %call.seq_reverse.elt39, 5
  store i8 %call.seq_reverse.elt39.elt49, ptr %new_node.repack38.repack48, align 1
  %new_node.repack38.repack50 = getelementptr inbounds nuw i8, ptr %new_node, i64 7
  %call.seq_reverse.elt39.elt51 = extractvalue [16 x i8] %call.seq_reverse.elt39, 6
  store i8 %call.seq_reverse.elt39.elt51, ptr %new_node.repack38.repack50, align 1
  %new_node.repack38.repack52 = getelementptr inbounds nuw i8, ptr %new_node, i64 8
  %call.seq_reverse.elt39.elt53 = extractvalue [16 x i8] %call.seq_reverse.elt39, 7
  store i8 %call.seq_reverse.elt39.elt53, ptr %new_node.repack38.repack52, align 1
  %new_node.repack38.repack54 = getelementptr inbounds nuw i8, ptr %new_node, i64 9
  %call.seq_reverse.elt39.elt55 = extractvalue [16 x i8] %call.seq_reverse.elt39, 8
  store i8 %call.seq_reverse.elt39.elt55, ptr %new_node.repack38.repack54, align 1
  %new_node.repack38.repack56 = getelementptr inbounds nuw i8, ptr %new_node, i64 10
  %call.seq_reverse.elt39.elt57 = extractvalue [16 x i8] %call.seq_reverse.elt39, 9
  store i8 %call.seq_reverse.elt39.elt57, ptr %new_node.repack38.repack56, align 1
  %new_node.repack38.repack58 = getelementptr inbounds nuw i8, ptr %new_node, i64 11
  %call.seq_reverse.elt39.elt59 = extractvalue [16 x i8] %call.seq_reverse.elt39, 10
  store i8 %call.seq_reverse.elt39.elt59, ptr %new_node.repack38.repack58, align 1
  %new_node.repack38.repack60 = getelementptr inbounds nuw i8, ptr %new_node, i64 12
  %call.seq_reverse.elt39.elt61 = extractvalue [16 x i8] %call.seq_reverse.elt39, 11
  store i8 %call.seq_reverse.elt39.elt61, ptr %new_node.repack38.repack60, align 1
  %new_node.repack38.repack62 = getelementptr inbounds nuw i8, ptr %new_node, i64 13
  %call.seq_reverse.elt39.elt63 = extractvalue [16 x i8] %call.seq_reverse.elt39, 12
  store i8 %call.seq_reverse.elt39.elt63, ptr %new_node.repack38.repack62, align 1
  %new_node.repack38.repack64 = getelementptr inbounds nuw i8, ptr %new_node, i64 14
  %call.seq_reverse.elt39.elt65 = extractvalue [16 x i8] %call.seq_reverse.elt39, 13
  store i8 %call.seq_reverse.elt39.elt65, ptr %new_node.repack38.repack64, align 1
  %new_node.repack38.repack66 = getelementptr inbounds nuw i8, ptr %new_node, i64 15
  %call.seq_reverse.elt39.elt67 = extractvalue [16 x i8] %call.seq_reverse.elt39, 14
  store i8 %call.seq_reverse.elt39.elt67, ptr %new_node.repack38.repack66, align 1
  %new_node.repack38.repack68 = getelementptr inbounds nuw i8, ptr %new_node, i64 16
  %call.seq_reverse.elt39.elt69 = extractvalue [16 x i8] %call.seq_reverse.elt39, 15
  store i8 %call.seq_reverse.elt39.elt69, ptr %new_node.repack38.repack68, align 1
  %next_ptr = getelementptr inbounds nuw i8, ptr %new_node, i64 24
  store ptr %.tr7073, ptr %next_ptr, align 8
  %is_null = icmp eq ptr %list_rest, null
  br i1 %is_null, label %common.ret, label %list_cons_test_elements
}

define { %Seq, %Seq } @conclude_current(%Seq %0, %Seq %1) local_unnamed_addr #0 {
entry:
  %tag = extractvalue %Seq %1, 0
  %eq_int = icmp eq i8 %tag, 3
  br i1 %eq_int, label %match.test.0, label %common.ret

match.test.0:                                     ; preds = %entry
  %extract_sum_type_payload = extractvalue %Seq %1, 1
  %union_cast_temp = alloca [16 x i8], align 1
  %extract_sum_type_payload.elt = extractvalue [16 x i8] %extract_sum_type_payload, 0
  store i8 %extract_sum_type_payload.elt, ptr %union_cast_temp, align 1
  %union_cast_temp.repack21 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 1
  %extract_sum_type_payload.elt22 = extractvalue [16 x i8] %extract_sum_type_payload, 1
  store i8 %extract_sum_type_payload.elt22, ptr %union_cast_temp.repack21, align 1
  %union_cast_temp.repack23 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 2
  %extract_sum_type_payload.elt24 = extractvalue [16 x i8] %extract_sum_type_payload, 2
  store i8 %extract_sum_type_payload.elt24, ptr %union_cast_temp.repack23, align 1
  %union_cast_temp.repack25 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 3
  %extract_sum_type_payload.elt26 = extractvalue [16 x i8] %extract_sum_type_payload, 3
  store i8 %extract_sum_type_payload.elt26, ptr %union_cast_temp.repack25, align 1
  %union_cast_temp.repack27 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 4
  %extract_sum_type_payload.elt28 = extractvalue [16 x i8] %extract_sum_type_payload, 4
  store i8 %extract_sum_type_payload.elt28, ptr %union_cast_temp.repack27, align 1
  %union_cast_temp.repack29 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 5
  %extract_sum_type_payload.elt30 = extractvalue [16 x i8] %extract_sum_type_payload, 5
  store i8 %extract_sum_type_payload.elt30, ptr %union_cast_temp.repack29, align 1
  %union_cast_temp.repack31 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 6
  %extract_sum_type_payload.elt32 = extractvalue [16 x i8] %extract_sum_type_payload, 6
  store i8 %extract_sum_type_payload.elt32, ptr %union_cast_temp.repack31, align 1
  %union_cast_temp.repack33 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 7
  %extract_sum_type_payload.elt34 = extractvalue [16 x i8] %extract_sum_type_payload, 7
  store i8 %extract_sum_type_payload.elt34, ptr %union_cast_temp.repack33, align 1
  %union_cast_temp.repack35 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 8
  %extract_sum_type_payload.elt36 = extractvalue [16 x i8] %extract_sum_type_payload, 8
  store i8 %extract_sum_type_payload.elt36, ptr %union_cast_temp.repack35, align 1
  %union_cast_temp.repack37 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 9
  %extract_sum_type_payload.elt38 = extractvalue [16 x i8] %extract_sum_type_payload, 9
  store i8 %extract_sum_type_payload.elt38, ptr %union_cast_temp.repack37, align 1
  %union_cast_temp.repack39 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 10
  %extract_sum_type_payload.elt40 = extractvalue [16 x i8] %extract_sum_type_payload, 10
  store i8 %extract_sum_type_payload.elt40, ptr %union_cast_temp.repack39, align 1
  %union_cast_temp.repack41 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 11
  %extract_sum_type_payload.elt42 = extractvalue [16 x i8] %extract_sum_type_payload, 11
  store i8 %extract_sum_type_payload.elt42, ptr %union_cast_temp.repack41, align 1
  %union_cast_temp.repack43 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 12
  %extract_sum_type_payload.elt44 = extractvalue [16 x i8] %extract_sum_type_payload, 12
  store i8 %extract_sum_type_payload.elt44, ptr %union_cast_temp.repack43, align 1
  %union_cast_temp.repack45 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 13
  %extract_sum_type_payload.elt46 = extractvalue [16 x i8] %extract_sum_type_payload, 13
  store i8 %extract_sum_type_payload.elt46, ptr %union_cast_temp.repack45, align 1
  %union_cast_temp.repack47 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 14
  %extract_sum_type_payload.elt48 = extractvalue [16 x i8] %extract_sum_type_payload, 14
  store i8 %extract_sum_type_payload.elt48, ptr %union_cast_temp.repack47, align 1
  %union_cast_temp.repack49 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 15
  %extract_sum_type_payload.elt50 = extractvalue [16 x i8] %extract_sum_type_payload, 15
  store i8 %extract_sum_type_payload.elt50, ptr %union_cast_temp.repack49, align 1
  %union_to_struct.unpack = load i32, ptr %union_cast_temp, align 8
  %union_to_struct.unpack52 = load i32, ptr %union_cast_temp.repack27, align 4
  %union_to_struct.unpack54 = load ptr, ptr %union_cast_temp.repack35, align 8
  %is_null = icmp eq ptr %union_to_struct.unpack54, null
  br i1 %is_null, label %list_cons_merge, label %list_cons_test_elements

common.ret:                                       ; preds = %entry, %list_cons_merge
  %call.seq_extend.pn = phi %Seq [ %call.seq_extend, %list_cons_merge ], [ %0, %entry ]
  %"insert variant data.pn" = phi %Seq [ %"insert variant data", %list_cons_merge ], [ %1, %entry ]
  %insertx.pn = insertvalue { %Seq, %Seq } undef, %Seq %call.seq_extend.pn, 0
  %common.ret.op = insertvalue { %Seq, %Seq } %insertx.pn, %Seq %"insert variant data.pn", 1
  ret { %Seq, %Seq } %common.ret.op

list_cons_test_elements:                          ; preds = %match.test.0
  %list_head.unpack = load i8, ptr %union_to_struct.unpack54, align 1
  %2 = insertvalue %Seq poison, i8 %list_head.unpack, 0
  %list_head.elt56 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 1
  %list_head.unpack57.unpack = load i8, ptr %list_head.elt56, align 1
  %3 = insertvalue [16 x i8] poison, i8 %list_head.unpack57.unpack, 0
  %list_head.unpack57.elt59 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 2
  %list_head.unpack57.unpack60 = load i8, ptr %list_head.unpack57.elt59, align 1
  %4 = insertvalue [16 x i8] %3, i8 %list_head.unpack57.unpack60, 1
  %list_head.unpack57.elt61 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 3
  %list_head.unpack57.unpack62 = load i8, ptr %list_head.unpack57.elt61, align 1
  %5 = insertvalue [16 x i8] %4, i8 %list_head.unpack57.unpack62, 2
  %list_head.unpack57.elt63 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 4
  %list_head.unpack57.unpack64 = load i8, ptr %list_head.unpack57.elt63, align 1
  %6 = insertvalue [16 x i8] %5, i8 %list_head.unpack57.unpack64, 3
  %list_head.unpack57.elt65 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 5
  %list_head.unpack57.unpack66 = load i8, ptr %list_head.unpack57.elt65, align 1
  %7 = insertvalue [16 x i8] %6, i8 %list_head.unpack57.unpack66, 4
  %list_head.unpack57.elt67 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 6
  %list_head.unpack57.unpack68 = load i8, ptr %list_head.unpack57.elt67, align 1
  %8 = insertvalue [16 x i8] %7, i8 %list_head.unpack57.unpack68, 5
  %list_head.unpack57.elt69 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 7
  %list_head.unpack57.unpack70 = load i8, ptr %list_head.unpack57.elt69, align 1
  %9 = insertvalue [16 x i8] %8, i8 %list_head.unpack57.unpack70, 6
  %list_head.unpack57.elt71 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 8
  %list_head.unpack57.unpack72 = load i8, ptr %list_head.unpack57.elt71, align 1
  %10 = insertvalue [16 x i8] %9, i8 %list_head.unpack57.unpack72, 7
  %list_head.unpack57.elt73 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 9
  %list_head.unpack57.unpack74 = load i8, ptr %list_head.unpack57.elt73, align 1
  %11 = insertvalue [16 x i8] %10, i8 %list_head.unpack57.unpack74, 8
  %list_head.unpack57.elt75 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 10
  %list_head.unpack57.unpack76 = load i8, ptr %list_head.unpack57.elt75, align 1
  %12 = insertvalue [16 x i8] %11, i8 %list_head.unpack57.unpack76, 9
  %list_head.unpack57.elt77 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 11
  %list_head.unpack57.unpack78 = load i8, ptr %list_head.unpack57.elt77, align 1
  %13 = insertvalue [16 x i8] %12, i8 %list_head.unpack57.unpack78, 10
  %list_head.unpack57.elt79 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 12
  %list_head.unpack57.unpack80 = load i8, ptr %list_head.unpack57.elt79, align 1
  %14 = insertvalue [16 x i8] %13, i8 %list_head.unpack57.unpack80, 11
  %list_head.unpack57.elt81 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 13
  %list_head.unpack57.unpack82 = load i8, ptr %list_head.unpack57.elt81, align 1
  %15 = insertvalue [16 x i8] %14, i8 %list_head.unpack57.unpack82, 12
  %list_head.unpack57.elt83 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 14
  %list_head.unpack57.unpack84 = load i8, ptr %list_head.unpack57.elt83, align 1
  %16 = insertvalue [16 x i8] %15, i8 %list_head.unpack57.unpack84, 13
  %list_head.unpack57.elt85 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 15
  %list_head.unpack57.unpack86 = load i8, ptr %list_head.unpack57.elt85, align 1
  %17 = insertvalue [16 x i8] %16, i8 %list_head.unpack57.unpack86, 14
  %list_head.unpack57.elt87 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 16
  %list_head.unpack57.unpack88 = load i8, ptr %list_head.unpack57.elt87, align 1
  %list_head.unpack5789 = insertvalue [16 x i8] %17, i8 %list_head.unpack57.unpack88, 15
  %list_head58 = insertvalue %Seq %2, [16 x i8] %list_head.unpack5789, 1
  %item_ptr1 = getelementptr inbounds nuw i8, ptr %union_to_struct.unpack54, i64 24
  %list_rest = load ptr, ptr %item_ptr1, align 8
  %18 = ptrtoint ptr %list_rest to i64
  %extract.t = trunc i64 %18 to i8
  %extract = lshr i64 %18, 8
  %extract.t125 = trunc i64 %extract to i8
  %extract126 = lshr i64 %18, 16
  %extract.t127 = trunc i64 %extract126 to i8
  %extract128 = lshr i64 %18, 24
  %extract.t129 = trunc i64 %extract128 to i8
  %extract130 = lshr i64 %18, 32
  %extract.t131 = trunc i64 %extract130 to i8
  %extract132 = lshr i64 %18, 40
  %extract.t133 = trunc i64 %extract132 to i8
  %extract134 = lshr i64 %18, 48
  %extract.t135 = trunc i64 %extract134 to i8
  %extract136 = lshr i64 %18, 56
  %extract.t137 = trunc nuw i64 %extract136 to i8
  br label %list_cons_merge

list_cons_merge:                                  ; preds = %list_cons_test_elements, %match.test.0
  %head_val = phi %Seq [ undef, %match.test.0 ], [ %list_head58, %list_cons_test_elements ]
  %tail_val.off0 = phi i8 [ undef, %match.test.0 ], [ %extract.t, %list_cons_test_elements ]
  %tail_val.off8 = phi i8 [ 0, %match.test.0 ], [ %extract.t125, %list_cons_test_elements ]
  %tail_val.off16 = phi i8 [ 0, %match.test.0 ], [ %extract.t127, %list_cons_test_elements ]
  %tail_val.off24 = phi i8 [ 0, %match.test.0 ], [ %extract.t129, %list_cons_test_elements ]
  %tail_val.off32 = phi i8 [ 0, %match.test.0 ], [ %extract.t131, %list_cons_test_elements ]
  %tail_val.off40 = phi i8 [ 0, %match.test.0 ], [ %extract.t133, %list_cons_test_elements ]
  %tail_val.off48 = phi i8 [ 0, %match.test.0 ], [ %extract.t135, %list_cons_test_elements ]
  %tail_val.off56 = phi i8 [ 0, %match.test.0 ], [ %extract.t137, %list_cons_test_elements ]
  %call.seq_extend = tail call %Seq @seq_extend(%Seq %head_val, %Seq %0)
  %-_int = add i32 %union_to_struct.unpack52, -1
  %19 = trunc i32 %union_to_struct.unpack to i8
  %20 = insertvalue [16 x i8] poison, i8 %19, 0
  %21 = lshr i32 %union_to_struct.unpack, 8
  %22 = trunc i32 %21 to i8
  %23 = insertvalue [16 x i8] %20, i8 %22, 1
  %24 = lshr i32 %union_to_struct.unpack, 16
  %25 = trunc i32 %24 to i8
  %26 = insertvalue [16 x i8] %23, i8 %25, 2
  %27 = lshr i32 %union_to_struct.unpack, 24
  %28 = trunc nuw i32 %27 to i8
  %29 = insertvalue [16 x i8] %26, i8 %28, 3
  %30 = trunc i32 %-_int to i8
  %31 = insertvalue [16 x i8] %29, i8 %30, 4
  %32 = lshr i32 %-_int, 8
  %33 = trunc i32 %32 to i8
  %34 = insertvalue [16 x i8] %31, i8 %33, 5
  %35 = lshr i32 %-_int, 16
  %36 = trunc i32 %35 to i8
  %37 = insertvalue [16 x i8] %34, i8 %36, 6
  %38 = lshr i32 %-_int, 24
  %39 = trunc nuw i32 %38 to i8
  %40 = insertvalue [16 x i8] %37, i8 %39, 7
  %41 = insertvalue [16 x i8] %40, i8 %tail_val.off0, 8
  %42 = insertvalue [16 x i8] %41, i8 %tail_val.off8, 9
  %43 = insertvalue [16 x i8] %42, i8 %tail_val.off16, 10
  %44 = insertvalue [16 x i8] %43, i8 %tail_val.off24, 11
  %45 = insertvalue [16 x i8] %44, i8 %tail_val.off32, 12
  %46 = insertvalue [16 x i8] %45, i8 %tail_val.off40, 13
  %47 = insertvalue [16 x i8] %46, i8 %tail_val.off48, 14
  %load_as_bytes124 = insertvalue [16 x i8] %47, i8 %tail_val.off56, 15
  %"insert variant data" = insertvalue %Seq { i8 3, [16 x i8] undef }, [16 x i8] %load_as_bytes124, 1
  br label %common.ret
}

define { i32, ptr } @copy_str_slice({ i32, ptr } %0) local_unnamed_addr #0 {
entry:
  %get_array_size = extractvalue { i32, ptr } %0, 0
  %"+_int" = add i32 %get_array_size, 1
  %call.malloc = tail call ptr @malloc(i32 %"+_int")
  %get_array_data_ptr = extractvalue { i32, ptr } %0, 1
  %call.memcpy = tail call ptr @memcpy(ptr %call.malloc, ptr %get_array_data_ptr, i32 %get_array_size)
  %insert_arr_size = insertvalue { i32, ptr } undef, i32 %get_array_size, 0
  %insert_arr_data = insertvalue { i32, ptr } %insert_arr_size, ptr %call.malloc, 1
  %1 = sext i32 %get_array_size to i64
  %element_ptr = getelementptr i8, ptr %call.malloc, i64 %1
  store i8 0, ptr %element_ptr, align 1
  ret { i32, ptr } %insert_arr_data
}

declare ptr @memcpy(ptr %0, ptr %1, i32 %2) local_unnamed_addr #0

define %Seq @parse_seq({ i32, ptr } %0, %Seq %1, %Seq %2) local_unnamed_addr #1 {
entry:
  %get_array_size = extractvalue { i32, ptr } %0, 0
  %">_int" = icmp sgt i32 %get_array_size, 0
  br i1 %">_int", label %match.body.0, label %common.ret

match.body.0:                                     ; preds = %entry
  %heap_array = tail call ptr @malloc(i32 38)
  store <16 x i8> <i8 94, i8 45, i8 63, i8 91, i8 48, i8 45, i8 57, i8 93, i8 42, i8 92, i8 46, i8 40, i8 91, i8 48, i8 45, i8 57>, ptr %heap_array, align 1
  %heap_array.repack352 = getelementptr inbounds nuw i8, ptr %heap_array, i64 16
  store <16 x i8> <i8 93, i8 42, i8 91, i8 49, i8 45, i8 57, i8 93, i8 124, i8 91, i8 48, i8 45, i8 57, i8 93, i8 91, i8 48, i8 45>, ptr %heap_array.repack352, align 1
  %heap_array.repack368 = getelementptr inbounds nuw i8, ptr %heap_array, i64 32
  store <4 x i8> <i8 57, i8 93, i8 42, i8 41>, ptr %heap_array.repack368, align 1
  %heap_array.repack372 = getelementptr inbounds nuw i8, ptr %heap_array, i64 36
  store i8 63, ptr %heap_array.repack372, align 1
  %heap_array.repack373 = getelementptr inbounds nuw i8, ptr %heap_array, i64 37
  store i8 0, ptr %heap_array.repack373, align 1
  %array_data_alloc.i = tail call ptr @malloc(i32 8)
  store i32 0, ptr %array_data_alloc.i, align 4
  %element_ptr1.i = getelementptr i8, ptr %array_data_alloc.i, i64 4
  store i32 0, ptr %element_ptr1.i, align 4
  %get_array_data_ptr.i = extractvalue { i32, ptr } %0, 1
  %call.regex_find_one.i = tail call i32 @regex_find_one(ptr %get_array_data_ptr.i, ptr nonnull %heap_array, ptr nonnull %array_data_alloc.i)
  %eq_int.not.not.i = icmp eq i32 %call.regex_find_one.i, 0
  br i1 %eq_int.not.not.i, label %match.body.1.i, label %find_one.exit

match.body.1.i:                                   ; preds = %match.body.0
  %element.i = load i32, ptr %array_data_alloc.i, align 4
  %insertx.i = insertvalue { i32, i32 } undef, i32 %element.i, 0
  %element10.i = load i32, ptr %element_ptr1.i, align 4
  %insertx11.i = insertvalue { i32, i32 } %insertx.i, i32 %element10.i, 1
  %"insert Some Value.i" = insertvalue { i8, { i32, i32 } } { i8 0, { i32, i32 } undef }, { i32, i32 } %insertx11.i, 1
  br label %find_one.exit

find_one.exit:                                    ; preds = %match.body.0, %match.body.1.i
  %common.ret.op.i = phi { i8, { i32, i32 } } [ %"insert Some Value.i", %match.body.1.i ], [ { i8 1, { i32, i32 } undef }, %match.body.0 ]
  %tag = extractvalue { i8, { i32, i32 } } %common.ret.op.i, 0
  %eq_int5 = icmp eq i8 %tag, 0
  br i1 %eq_int5, label %match.test.01, label %match.body.14

common.ret:                                       ; preds = %entry, %match.merge
  %common.ret.op = phi %Seq [ %call.parse_seq, %match.merge ], [ %1, %entry ]
  ret %Seq %common.ret.op

match.merge:                                      ; preds = %match.test.0304, %match.body.1308, %match.test.0270, %match.test.0234, %match.test.0200, %match.test.0164, %match.test.0131, %match.test.097, %match.test.058, %match.test.019, %match.test.01
  %insert_new_data_ptr9.pn = phi { i32, ptr } [ %insert_new_data_ptr9, %match.test.01 ], [ %insert_new_data_ptr47, %match.test.019 ], [ %insert_new_data_ptr86, %match.test.058 ], [ %insert_new_data_ptr117, %match.test.097 ], [ %insert_new_data_ptr151, %match.test.0131 ], [ %insert_new_data_ptr184, %match.test.0164 ], [ %insert_new_data_ptr220, %match.test.0200 ], [ %insert_new_data_ptr254, %match.test.0234 ], [ %insert_new_data_ptr290, %match.test.0270 ], [ %insert_new_data_ptr324, %match.test.0304 ], [ %insert_array_size330, %match.body.1308 ]
  %call.seq_extend.pn = phi %Seq [ %call.seq_extend, %match.test.01 ], [ %call.seq_extend38, %match.test.019 ], [ %call.seq_extend77, %match.test.058 ], [ %"insert variant data123", %match.test.097 ], [ %struct_field_0152, %match.test.0131 ], [ %"insert variant data191", %match.test.0164 ], [ %struct_field_0222, %match.test.0200 ], [ %"insert variant data261", %match.test.0234 ], [ %struct_field_0292, %match.test.0270 ], [ %1, %match.test.0304 ], [ %1, %match.body.1308 ]
  %.pn = phi %Seq [ %2, %match.test.01 ], [ %2, %match.test.019 ], [ %2, %match.test.058 ], [ %call.seq_extend.i, %match.test.097 ], [ %struct_field_1153, %match.test.0131 ], [ %call.seq_extend.i691, %match.test.0164 ], [ %struct_field_1223, %match.test.0200 ], [ %call.seq_extend.i708, %match.test.0234 ], [ %struct_field_1293, %match.test.0270 ], [ %2, %match.test.0304 ], [ %2, %match.body.1308 ]
  %call.parse_seq = tail call %Seq @parse_seq({ i32, ptr } %insert_new_data_ptr9.pn, %Seq %call.seq_extend.pn, %Seq %.pn)
  br label %common.ret

match.test.01:                                    ; preds = %find_one.exit
  %extract_sum_type_payload = extractvalue { i8, { i32, i32 } } %common.ret.op.i, 1
  %struct_field_0 = extractvalue { i32, i32 } %extract_sum_type_payload, 0
  %struct_field_1 = extractvalue { i32, i32 } %extract_sum_type_payload, 1
  %3 = sext i32 %struct_field_0 to i64
  %new_data_ptr = getelementptr i8, ptr %get_array_data_ptr.i, i64 %3
  %insert_new_size = insertvalue { i32, ptr } undef, i32 %struct_field_1, 0
  %insert_new_data_ptr = insertvalue { i32, ptr } %insert_new_size, ptr %new_data_ptr, 1
  %call.double_parse = tail call double @double_parse({ i32, ptr } %insert_new_data_ptr)
  %4 = bitcast double %call.double_parse to i64
  %5 = trunc i64 %4 to i8
  %6 = insertvalue [16 x i8] poison, i8 %5, 0
  %7 = lshr i64 %4, 8
  %8 = trunc i64 %7 to i8
  %9 = insertvalue [16 x i8] %6, i8 %8, 1
  %10 = lshr i64 %4, 16
  %11 = trunc i64 %10 to i8
  %12 = insertvalue [16 x i8] %9, i8 %11, 2
  %13 = lshr i64 %4, 24
  %14 = trunc i64 %13 to i8
  %15 = insertvalue [16 x i8] %12, i8 %14, 3
  %16 = lshr i64 %4, 32
  %17 = trunc i64 %16 to i8
  %18 = insertvalue [16 x i8] %15, i8 %17, 4
  %19 = lshr i64 %4, 40
  %20 = trunc i64 %19 to i8
  %21 = insertvalue [16 x i8] %18, i8 %20, 5
  %22 = lshr i64 %4, 48
  %23 = trunc i64 %22 to i8
  %24 = insertvalue [16 x i8] %21, i8 %23, 6
  %25 = lshr i64 %4, 56
  %26 = trunc nuw i64 %25 to i8
  %27 = insertvalue [16 x i8] %24, i8 %26, 7
  %28 = insertvalue [16 x i8] %27, i8 undef, 8
  %29 = insertvalue [16 x i8] %28, i8 undef, 9
  %30 = insertvalue [16 x i8] %29, i8 undef, 10
  %31 = insertvalue [16 x i8] %30, i8 undef, 11
  %32 = insertvalue [16 x i8] %31, i8 undef, 12
  %33 = insertvalue [16 x i8] %32, i8 undef, 13
  %34 = insertvalue [16 x i8] %33, i8 undef, 14
  %load_as_bytes623 = insertvalue [16 x i8] %34, i8 undef, 15
  %"insert variant data" = insertvalue %Seq { i8 1, [16 x i8] undef }, [16 x i8] %load_as_bytes623, 1
  %call.seq_extend = tail call %Seq @seq_extend(%Seq %1, %Seq %"insert variant data")
  %new_size = sub i32 %get_array_size, %struct_field_1
  %35 = sext i32 %struct_field_1 to i64
  %new_data_ptr7 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %35
  %insert_new_size8 = insertvalue { i32, ptr } undef, i32 %new_size, 0
  %insert_new_data_ptr9 = insertvalue { i32, ptr } %insert_new_size8, ptr %new_data_ptr7, 1
  br label %match.merge

match.body.14:                                    ; preds = %find_one.exit
  %heap_array12 = tail call ptr @malloc(i32 8)
  store <8 x i8> <i8 94, i8 91, i8 48, i8 45, i8 57, i8 93, i8 43, i8 0>, ptr %heap_array12, align 1
  %array_data_alloc.i624 = tail call ptr @malloc(i32 8)
  store i32 0, ptr %array_data_alloc.i624, align 4
  %element_ptr1.i625 = getelementptr i8, ptr %array_data_alloc.i624, i64 4
  store i32 0, ptr %element_ptr1.i625, align 4
  %call.regex_find_one.i627 = tail call i32 @regex_find_one(ptr %get_array_data_ptr.i, ptr nonnull %heap_array12, ptr nonnull %array_data_alloc.i624)
  %eq_int.not.not.i628 = icmp eq i32 %call.regex_find_one.i627, 0
  br i1 %eq_int.not.not.i628, label %match.body.1.i630, label %find_one.exit636

match.body.1.i630:                                ; preds = %match.body.14
  %element.i631 = load i32, ptr %array_data_alloc.i624, align 4
  %insertx.i632 = insertvalue { i32, i32 } undef, i32 %element.i631, 0
  %element10.i633 = load i32, ptr %element_ptr1.i625, align 4
  %insertx11.i634 = insertvalue { i32, i32 } %insertx.i632, i32 %element10.i633, 1
  %"insert Some Value.i635" = insertvalue { i8, { i32, i32 } } { i8 0, { i32, i32 } undef }, { i32, i32 } %insertx11.i634, 1
  br label %find_one.exit636

find_one.exit636:                                 ; preds = %match.body.14, %match.body.1.i630
  %common.ret.op.i629 = phi { i8, { i32, i32 } } [ %"insert Some Value.i635", %match.body.1.i630 ], [ { i8 1, { i32, i32 } undef }, %match.body.14 ]
  %tag24 = extractvalue { i8, { i32, i32 } } %common.ret.op.i629, 0
  %eq_int25 = icmp eq i8 %tag24, 0
  br i1 %eq_int25, label %match.test.019, label %match.body.123

match.test.019:                                   ; preds = %find_one.exit636
  %extract_sum_type_payload26 = extractvalue { i8, { i32, i32 } } %common.ret.op.i629, 1
  %struct_field_029 = extractvalue { i32, i32 } %extract_sum_type_payload26, 0
  %struct_field_130 = extractvalue { i32, i32 } %extract_sum_type_payload26, 1
  %36 = sext i32 %struct_field_029 to i64
  %new_data_ptr32 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %36
  %insert_new_size33 = insertvalue { i32, ptr } undef, i32 %struct_field_130, 0
  %insert_new_data_ptr34 = insertvalue { i32, ptr } %insert_new_size33, ptr %new_data_ptr32, 1
  %call.int32_parse = tail call i32 @int32_parse({ i32, ptr } %insert_new_data_ptr34)
  %37 = trunc i32 %call.int32_parse to i8
  %38 = insertvalue [16 x i8] poison, i8 %37, 0
  %39 = lshr i32 %call.int32_parse, 8
  %40 = trunc i32 %39 to i8
  %41 = insertvalue [16 x i8] %38, i8 %40, 1
  %42 = lshr i32 %call.int32_parse, 16
  %43 = trunc i32 %42 to i8
  %44 = insertvalue [16 x i8] %41, i8 %43, 2
  %45 = lshr i32 %call.int32_parse, 24
  %46 = trunc nuw i32 %45 to i8
  %47 = insertvalue [16 x i8] %44, i8 %46, 3
  %48 = insertvalue [16 x i8] %47, i8 undef, 4
  %49 = insertvalue [16 x i8] %48, i8 undef, 5
  %50 = insertvalue [16 x i8] %49, i8 undef, 6
  %51 = insertvalue [16 x i8] %50, i8 undef, 7
  %52 = insertvalue [16 x i8] %51, i8 undef, 8
  %53 = insertvalue [16 x i8] %52, i8 undef, 9
  %54 = insertvalue [16 x i8] %53, i8 undef, 10
  %55 = insertvalue [16 x i8] %54, i8 undef, 11
  %56 = insertvalue [16 x i8] %55, i8 undef, 12
  %57 = insertvalue [16 x i8] %56, i8 undef, 13
  %58 = insertvalue [16 x i8] %57, i8 undef, 14
  %load_as_bytes36592 = insertvalue [16 x i8] %58, i8 undef, 15
  %"insert variant data37" = insertvalue %Seq { i8 0, [16 x i8] undef }, [16 x i8] %load_as_bytes36592, 1
  %call.seq_extend38 = tail call %Seq @seq_extend(%Seq %1, %Seq %"insert variant data37")
  %new_size43 = sub i32 %get_array_size, %struct_field_130
  %59 = sext i32 %struct_field_130 to i64
  %new_data_ptr45 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %59
  %insert_new_size46 = insertvalue { i32, ptr } undef, i32 %new_size43, 0
  %insert_new_data_ptr47 = insertvalue { i32, ptr } %insert_new_size46, ptr %new_data_ptr45, 1
  br label %match.merge

match.body.123:                                   ; preds = %find_one.exit636
  %heap_array51 = tail call ptr @malloc(i32 25)
  store <16 x i8> <i8 94, i8 91, i8 97, i8 45, i8 122, i8 65, i8 45, i8 90, i8 95, i8 93, i8 91, i8 97, i8 45, i8 122, i8 65, i8 45>, ptr %heap_array51, align 1
  %heap_array51.repack396 = getelementptr inbounds nuw i8, ptr %heap_array51, i64 16
  store <8 x i8> <i8 90, i8 48, i8 45, i8 57, i8 95, i8 35, i8 93, i8 42>, ptr %heap_array51.repack396, align 1
  %heap_array51.repack404 = getelementptr inbounds nuw i8, ptr %heap_array51, i64 24
  store i8 0, ptr %heap_array51.repack404, align 1
  %array_data_alloc.i637 = tail call ptr @malloc(i32 8)
  store i32 0, ptr %array_data_alloc.i637, align 4
  %element_ptr1.i638 = getelementptr i8, ptr %array_data_alloc.i637, i64 4
  store i32 0, ptr %element_ptr1.i638, align 4
  %call.regex_find_one.i640 = tail call i32 @regex_find_one(ptr %get_array_data_ptr.i, ptr nonnull %heap_array51, ptr nonnull %array_data_alloc.i637)
  %eq_int.not.not.i641 = icmp eq i32 %call.regex_find_one.i640, 0
  br i1 %eq_int.not.not.i641, label %match.body.1.i643, label %find_one.exit649

match.body.1.i643:                                ; preds = %match.body.123
  %element.i644 = load i32, ptr %array_data_alloc.i637, align 4
  %insertx.i645 = insertvalue { i32, i32 } undef, i32 %element.i644, 0
  %element10.i646 = load i32, ptr %element_ptr1.i638, align 4
  %insertx11.i647 = insertvalue { i32, i32 } %insertx.i645, i32 %element10.i646, 1
  %"insert Some Value.i648" = insertvalue { i8, { i32, i32 } } { i8 0, { i32, i32 } undef }, { i32, i32 } %insertx11.i647, 1
  br label %find_one.exit649

find_one.exit649:                                 ; preds = %match.body.123, %match.body.1.i643
  %common.ret.op.i642 = phi { i8, { i32, i32 } } [ %"insert Some Value.i648", %match.body.1.i643 ], [ { i8 1, { i32, i32 } undef }, %match.body.123 ]
  %tag63 = extractvalue { i8, { i32, i32 } } %common.ret.op.i642, 0
  %eq_int64 = icmp eq i8 %tag63, 0
  br i1 %eq_int64, label %match.test.058, label %match.body.162

match.test.058:                                   ; preds = %find_one.exit649
  %extract_sum_type_payload65 = extractvalue { i8, { i32, i32 } } %common.ret.op.i642, 1
  %struct_field_068 = extractvalue { i32, i32 } %extract_sum_type_payload65, 0
  %struct_field_169 = extractvalue { i32, i32 } %extract_sum_type_payload65, 1
  %60 = sext i32 %struct_field_068 to i64
  %new_data_ptr71 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %60
  %insert_new_size72 = insertvalue { i32, ptr } undef, i32 %struct_field_169, 0
  %"+_int.i" = add i32 %struct_field_169, 1
  %call.malloc.i = tail call ptr @malloc(i32 %"+_int.i")
  %call.memcpy.i = tail call ptr @memcpy(ptr %call.malloc.i, ptr %new_data_ptr71, i32 %struct_field_169)
  %insert_arr_data.i = insertvalue { i32, ptr } %insert_new_size72, ptr %call.malloc.i, 1
  %61 = sext i32 %struct_field_169 to i64
  %element_ptr.i = getelementptr i8, ptr %call.malloc.i, i64 %61
  store i8 0, ptr %element_ptr.i, align 1
  %val_temp74 = alloca { i32, ptr }, align 8
  store { i32, ptr } %insert_arr_data.i, ptr %val_temp74, align 8
  %load_as_bytes75.unpack = load i8, ptr %val_temp74, align 8
  %62 = insertvalue [16 x i8] poison, i8 %load_as_bytes75.unpack, 0
  %load_as_bytes75.elt531 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 1
  %load_as_bytes75.unpack532 = load i8, ptr %load_as_bytes75.elt531, align 1
  %63 = insertvalue [16 x i8] %62, i8 %load_as_bytes75.unpack532, 1
  %load_as_bytes75.elt533 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 2
  %load_as_bytes75.unpack534 = load i8, ptr %load_as_bytes75.elt533, align 2
  %64 = insertvalue [16 x i8] %63, i8 %load_as_bytes75.unpack534, 2
  %load_as_bytes75.elt535 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 3
  %load_as_bytes75.unpack536 = load i8, ptr %load_as_bytes75.elt535, align 1
  %65 = insertvalue [16 x i8] %64, i8 %load_as_bytes75.unpack536, 3
  %load_as_bytes75.elt537 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 4
  %load_as_bytes75.unpack538 = load i8, ptr %load_as_bytes75.elt537, align 4
  %66 = insertvalue [16 x i8] %65, i8 %load_as_bytes75.unpack538, 4
  %load_as_bytes75.elt539 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 5
  %load_as_bytes75.unpack540 = load i8, ptr %load_as_bytes75.elt539, align 1
  %67 = insertvalue [16 x i8] %66, i8 %load_as_bytes75.unpack540, 5
  %load_as_bytes75.elt541 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 6
  %load_as_bytes75.unpack542 = load i8, ptr %load_as_bytes75.elt541, align 2
  %68 = insertvalue [16 x i8] %67, i8 %load_as_bytes75.unpack542, 6
  %load_as_bytes75.elt543 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 7
  %load_as_bytes75.unpack544 = load i8, ptr %load_as_bytes75.elt543, align 1
  %69 = insertvalue [16 x i8] %68, i8 %load_as_bytes75.unpack544, 7
  %load_as_bytes75.elt545 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 8
  %load_as_bytes75.unpack546 = load i8, ptr %load_as_bytes75.elt545, align 8
  %70 = insertvalue [16 x i8] %69, i8 %load_as_bytes75.unpack546, 8
  %load_as_bytes75.elt547 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 9
  %load_as_bytes75.unpack548 = load i8, ptr %load_as_bytes75.elt547, align 1
  %71 = insertvalue [16 x i8] %70, i8 %load_as_bytes75.unpack548, 9
  %load_as_bytes75.elt549 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 10
  %load_as_bytes75.unpack550 = load i8, ptr %load_as_bytes75.elt549, align 2
  %72 = insertvalue [16 x i8] %71, i8 %load_as_bytes75.unpack550, 10
  %load_as_bytes75.elt551 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 11
  %load_as_bytes75.unpack552 = load i8, ptr %load_as_bytes75.elt551, align 1
  %73 = insertvalue [16 x i8] %72, i8 %load_as_bytes75.unpack552, 11
  %load_as_bytes75.elt553 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 12
  %load_as_bytes75.unpack554 = load i8, ptr %load_as_bytes75.elt553, align 4
  %74 = insertvalue [16 x i8] %73, i8 %load_as_bytes75.unpack554, 12
  %load_as_bytes75.elt555 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 13
  %load_as_bytes75.unpack556 = load i8, ptr %load_as_bytes75.elt555, align 1
  %75 = insertvalue [16 x i8] %74, i8 %load_as_bytes75.unpack556, 13
  %load_as_bytes75.elt557 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 14
  %load_as_bytes75.unpack558 = load i8, ptr %load_as_bytes75.elt557, align 2
  %76 = insertvalue [16 x i8] %75, i8 %load_as_bytes75.unpack558, 14
  %load_as_bytes75.elt559 = getelementptr inbounds nuw i8, ptr %val_temp74, i64 15
  %load_as_bytes75.unpack560 = load i8, ptr %load_as_bytes75.elt559, align 1
  %load_as_bytes75561 = insertvalue [16 x i8] %76, i8 %load_as_bytes75.unpack560, 15
  %"insert variant data76" = insertvalue %Seq { i8 2, [16 x i8] undef }, [16 x i8] %load_as_bytes75561, 1
  %call.seq_extend77 = tail call %Seq @seq_extend(%Seq %1, %Seq %"insert variant data76")
  %new_size82 = sub i32 %get_array_size, %struct_field_169
  %new_data_ptr84 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %61
  %insert_new_size85 = insertvalue { i32, ptr } undef, i32 %new_size82, 0
  %insert_new_data_ptr86 = insertvalue { i32, ptr } %insert_new_size85, ptr %new_data_ptr84, 1
  br label %match.merge

match.body.162:                                   ; preds = %find_one.exit649
  %heap_array90 = tail call ptr @malloc(i32 4)
  store <4 x i8> <i8 94, i8 92, i8 123, i8 0>, ptr %heap_array90, align 1
  %array_data_alloc.i651 = tail call ptr @malloc(i32 8)
  store i32 0, ptr %array_data_alloc.i651, align 4
  %element_ptr1.i652 = getelementptr i8, ptr %array_data_alloc.i651, i64 4
  store i32 0, ptr %element_ptr1.i652, align 4
  %call.regex_find_one.i654 = tail call i32 @regex_find_one(ptr %get_array_data_ptr.i, ptr nonnull %heap_array90, ptr nonnull %array_data_alloc.i651)
  %eq_int.not.not.i655 = icmp eq i32 %call.regex_find_one.i654, 0
  br i1 %eq_int.not.not.i655, label %match.body.1.i657, label %find_one.exit663

match.body.1.i657:                                ; preds = %match.body.162
  %element.i658 = load i32, ptr %array_data_alloc.i651, align 4
  %insertx.i659 = insertvalue { i32, i32 } undef, i32 %element.i658, 0
  %element10.i660 = load i32, ptr %element_ptr1.i652, align 4
  %insertx11.i661 = insertvalue { i32, i32 } %insertx.i659, i32 %element10.i660, 1
  %"insert Some Value.i662" = insertvalue { i8, { i32, i32 } } { i8 0, { i32, i32 } undef }, { i32, i32 } %insertx11.i661, 1
  br label %find_one.exit663

find_one.exit663:                                 ; preds = %match.body.162, %match.body.1.i657
  %common.ret.op.i656 = phi { i8, { i32, i32 } } [ %"insert Some Value.i662", %match.body.1.i657 ], [ { i8 1, { i32, i32 } undef }, %match.body.162 ]
  %tag102 = extractvalue { i8, { i32, i32 } } %common.ret.op.i656, 0
  %eq_int103 = icmp eq i8 %tag102, 0
  br i1 %eq_int103, label %match.test.097, label %match.body.1101

match.test.097:                                   ; preds = %find_one.exit663
  %extract_sum_type_payload104 = extractvalue { i8, { i32, i32 } } %common.ret.op.i656, 1
  %struct_field_1108 = extractvalue { i32, i32 } %extract_sum_type_payload104, 1
  %new_size113 = sub i32 %get_array_size, %struct_field_1108
  %77 = sext i32 %struct_field_1108 to i64
  %new_data_ptr115 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %77
  %insert_new_size116 = insertvalue { i32, ptr } undef, i32 %new_size113, 0
  %insert_new_data_ptr117 = insertvalue { i32, ptr } %insert_new_size116, ptr %new_data_ptr115, 1
  %void_ptr = load ptr, ptr getelementptr inbounds nuw (i8, ptr @global_storage_array, i64 8), align 8
  %SEQ_CHOOSE_TYPE_load = load i32, ptr %void_ptr, align 4
  %78 = trunc i32 %SEQ_CHOOSE_TYPE_load to i8
  %79 = insertvalue [16 x i8] poison, i8 %78, 0
  %80 = lshr i32 %SEQ_CHOOSE_TYPE_load, 8
  %81 = trunc i32 %80 to i8
  %82 = insertvalue [16 x i8] %79, i8 %81, 1
  %83 = lshr i32 %SEQ_CHOOSE_TYPE_load, 16
  %84 = trunc i32 %83 to i8
  %85 = insertvalue [16 x i8] %82, i8 %84, 2
  %86 = lshr i32 %SEQ_CHOOSE_TYPE_load, 24
  %87 = trunc nuw i32 %86 to i8
  %88 = insertvalue [16 x i8] %85, i8 %87, 3
  %89 = insertvalue [16 x i8] %88, i8 0, 4
  %90 = insertvalue [16 x i8] %89, i8 0, 5
  %91 = insertvalue [16 x i8] %90, i8 0, 6
  %92 = insertvalue [16 x i8] %91, i8 0, 7
  %93 = insertvalue [16 x i8] %92, i8 0, 8
  %94 = insertvalue [16 x i8] %93, i8 0, 9
  %95 = insertvalue [16 x i8] %94, i8 0, 10
  %96 = insertvalue [16 x i8] %95, i8 0, 11
  %97 = insertvalue [16 x i8] %96, i8 0, 12
  %98 = insertvalue [16 x i8] %97, i8 0, 13
  %99 = insertvalue [16 x i8] %98, i8 0, 14
  %load_as_bytes122530 = insertvalue [16 x i8] %99, i8 0, 15
  %"insert variant data123" = insertvalue %Seq { i8 3, [16 x i8] undef }, [16 x i8] %load_as_bytes122530, 1
  %call.seq_extend.i = tail call %Seq @seq_extend(%Seq %2, %Seq %1)
  br label %match.merge

match.body.1101:                                  ; preds = %find_one.exit663
  %heap_array124 = tail call ptr @malloc(i32 4)
  store <4 x i8> <i8 94, i8 92, i8 125, i8 0>, ptr %heap_array124, align 1
  %array_data_alloc.i665 = tail call ptr @malloc(i32 8)
  store i32 0, ptr %array_data_alloc.i665, align 4
  %element_ptr1.i666 = getelementptr i8, ptr %array_data_alloc.i665, i64 4
  store i32 0, ptr %element_ptr1.i666, align 4
  %call.regex_find_one.i668 = tail call i32 @regex_find_one(ptr %get_array_data_ptr.i, ptr nonnull %heap_array124, ptr nonnull %array_data_alloc.i665)
  %eq_int.not.not.i669 = icmp eq i32 %call.regex_find_one.i668, 0
  br i1 %eq_int.not.not.i669, label %match.body.1.i671, label %find_one.exit677

match.body.1.i671:                                ; preds = %match.body.1101
  %element.i672 = load i32, ptr %array_data_alloc.i665, align 4
  %insertx.i673 = insertvalue { i32, i32 } undef, i32 %element.i672, 0
  %element10.i674 = load i32, ptr %element_ptr1.i666, align 4
  %insertx11.i675 = insertvalue { i32, i32 } %insertx.i673, i32 %element10.i674, 1
  %"insert Some Value.i676" = insertvalue { i8, { i32, i32 } } { i8 0, { i32, i32 } undef }, { i32, i32 } %insertx11.i675, 1
  br label %find_one.exit677

find_one.exit677:                                 ; preds = %match.body.1101, %match.body.1.i671
  %common.ret.op.i670 = phi { i8, { i32, i32 } } [ %"insert Some Value.i676", %match.body.1.i671 ], [ { i8 1, { i32, i32 } undef }, %match.body.1101 ]
  %tag136 = extractvalue { i8, { i32, i32 } } %common.ret.op.i670, 0
  %eq_int137 = icmp eq i8 %tag136, 0
  br i1 %eq_int137, label %match.test.0131, label %match.body.1135

match.test.0131:                                  ; preds = %find_one.exit677
  %extract_sum_type_payload138 = extractvalue { i8, { i32, i32 } } %common.ret.op.i670, 1
  %struct_field_1142 = extractvalue { i32, i32 } %extract_sum_type_payload138, 1
  %new_size147 = sub i32 %get_array_size, %struct_field_1142
  %100 = sext i32 %struct_field_1142 to i64
  %new_data_ptr149 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %100
  %insert_new_size150 = insertvalue { i32, ptr } undef, i32 %new_size147, 0
  %insert_new_data_ptr151 = insertvalue { i32, ptr } %insert_new_size150, ptr %new_data_ptr149, 1
  %call.conclude_current = tail call { %Seq, %Seq } @conclude_current(%Seq %1, %Seq %2)
  %struct_field_0152 = extractvalue { %Seq, %Seq } %call.conclude_current, 0
  %struct_field_1153 = extractvalue { %Seq, %Seq } %call.conclude_current, 1
  br label %match.merge

match.body.1135:                                  ; preds = %find_one.exit677
  %heap_array157 = tail call ptr @malloc(i32 4)
  store <4 x i8> <i8 94, i8 92, i8 91, i8 0>, ptr %heap_array157, align 1
  %array_data_alloc.i678 = tail call ptr @malloc(i32 8)
  store i32 0, ptr %array_data_alloc.i678, align 4
  %element_ptr1.i679 = getelementptr i8, ptr %array_data_alloc.i678, i64 4
  store i32 0, ptr %element_ptr1.i679, align 4
  %call.regex_find_one.i681 = tail call i32 @regex_find_one(ptr %get_array_data_ptr.i, ptr nonnull %heap_array157, ptr nonnull %array_data_alloc.i678)
  %eq_int.not.not.i682 = icmp eq i32 %call.regex_find_one.i681, 0
  br i1 %eq_int.not.not.i682, label %match.body.1.i684, label %find_one.exit690

match.body.1.i684:                                ; preds = %match.body.1135
  %element.i685 = load i32, ptr %array_data_alloc.i678, align 4
  %insertx.i686 = insertvalue { i32, i32 } undef, i32 %element.i685, 0
  %element10.i687 = load i32, ptr %element_ptr1.i679, align 4
  %insertx11.i688 = insertvalue { i32, i32 } %insertx.i686, i32 %element10.i687, 1
  %"insert Some Value.i689" = insertvalue { i8, { i32, i32 } } { i8 0, { i32, i32 } undef }, { i32, i32 } %insertx11.i688, 1
  br label %find_one.exit690

find_one.exit690:                                 ; preds = %match.body.1135, %match.body.1.i684
  %common.ret.op.i683 = phi { i8, { i32, i32 } } [ %"insert Some Value.i689", %match.body.1.i684 ], [ { i8 1, { i32, i32 } undef }, %match.body.1135 ]
  %tag169 = extractvalue { i8, { i32, i32 } } %common.ret.op.i683, 0
  %eq_int170 = icmp eq i8 %tag169, 0
  br i1 %eq_int170, label %match.test.0164, label %match.body.1168

match.test.0164:                                  ; preds = %find_one.exit690
  %extract_sum_type_payload171 = extractvalue { i8, { i32, i32 } } %common.ret.op.i683, 1
  %struct_field_1175 = extractvalue { i32, i32 } %extract_sum_type_payload171, 1
  %new_size180 = sub i32 %get_array_size, %struct_field_1175
  %101 = sext i32 %struct_field_1175 to i64
  %new_data_ptr182 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %101
  %insert_new_size183 = insertvalue { i32, ptr } undef, i32 %new_size180, 0
  %insert_new_data_ptr184 = insertvalue { i32, ptr } %insert_new_size183, ptr %new_data_ptr182, 1
  %void_ptr185 = load ptr, ptr @global_storage_array, align 8
  %SEQ_LIST_TYPE_load = load i32, ptr %void_ptr185, align 4
  %102 = trunc i32 %SEQ_LIST_TYPE_load to i8
  %103 = insertvalue [16 x i8] poison, i8 %102, 0
  %104 = lshr i32 %SEQ_LIST_TYPE_load, 8
  %105 = trunc i32 %104 to i8
  %106 = insertvalue [16 x i8] %103, i8 %105, 1
  %107 = lshr i32 %SEQ_LIST_TYPE_load, 16
  %108 = trunc i32 %107 to i8
  %109 = insertvalue [16 x i8] %106, i8 %108, 2
  %110 = lshr i32 %SEQ_LIST_TYPE_load, 24
  %111 = trunc nuw i32 %110 to i8
  %112 = insertvalue [16 x i8] %109, i8 %111, 3
  %113 = insertvalue [16 x i8] %112, i8 0, 4
  %114 = insertvalue [16 x i8] %113, i8 0, 5
  %115 = insertvalue [16 x i8] %114, i8 0, 6
  %116 = insertvalue [16 x i8] %115, i8 0, 7
  %117 = insertvalue [16 x i8] %116, i8 0, 8
  %118 = insertvalue [16 x i8] %117, i8 0, 9
  %119 = insertvalue [16 x i8] %118, i8 0, 10
  %120 = insertvalue [16 x i8] %119, i8 0, 11
  %121 = insertvalue [16 x i8] %120, i8 0, 12
  %122 = insertvalue [16 x i8] %121, i8 0, 13
  %123 = insertvalue [16 x i8] %122, i8 0, 14
  %load_as_bytes190495 = insertvalue [16 x i8] %123, i8 0, 15
  %"insert variant data191" = insertvalue %Seq { i8 3, [16 x i8] undef }, [16 x i8] %load_as_bytes190495, 1
  %call.seq_extend.i691 = tail call %Seq @seq_extend(%Seq %2, %Seq %1)
  br label %match.merge

match.body.1168:                                  ; preds = %find_one.exit690
  %heap_array193 = tail call ptr @malloc(i32 4)
  store <4 x i8> <i8 94, i8 92, i8 93, i8 0>, ptr %heap_array193, align 1
  %array_data_alloc.i695 = tail call ptr @malloc(i32 8)
  store i32 0, ptr %array_data_alloc.i695, align 4
  %element_ptr1.i696 = getelementptr i8, ptr %array_data_alloc.i695, i64 4
  store i32 0, ptr %element_ptr1.i696, align 4
  %call.regex_find_one.i698 = tail call i32 @regex_find_one(ptr %get_array_data_ptr.i, ptr nonnull %heap_array193, ptr nonnull %array_data_alloc.i695)
  %eq_int.not.not.i699 = icmp eq i32 %call.regex_find_one.i698, 0
  br i1 %eq_int.not.not.i699, label %match.body.1.i701, label %find_one.exit707

match.body.1.i701:                                ; preds = %match.body.1168
  %element.i702 = load i32, ptr %array_data_alloc.i695, align 4
  %insertx.i703 = insertvalue { i32, i32 } undef, i32 %element.i702, 0
  %element10.i704 = load i32, ptr %element_ptr1.i696, align 4
  %insertx11.i705 = insertvalue { i32, i32 } %insertx.i703, i32 %element10.i704, 1
  %"insert Some Value.i706" = insertvalue { i8, { i32, i32 } } { i8 0, { i32, i32 } undef }, { i32, i32 } %insertx11.i705, 1
  br label %find_one.exit707

find_one.exit707:                                 ; preds = %match.body.1168, %match.body.1.i701
  %common.ret.op.i700 = phi { i8, { i32, i32 } } [ %"insert Some Value.i706", %match.body.1.i701 ], [ { i8 1, { i32, i32 } undef }, %match.body.1168 ]
  %tag205 = extractvalue { i8, { i32, i32 } } %common.ret.op.i700, 0
  %eq_int206 = icmp eq i8 %tag205, 0
  br i1 %eq_int206, label %match.test.0200, label %match.body.1204

match.test.0200:                                  ; preds = %find_one.exit707
  %extract_sum_type_payload207 = extractvalue { i8, { i32, i32 } } %common.ret.op.i700, 1
  %struct_field_1211 = extractvalue { i32, i32 } %extract_sum_type_payload207, 1
  %new_size216 = sub i32 %get_array_size, %struct_field_1211
  %124 = sext i32 %struct_field_1211 to i64
  %new_data_ptr218 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %124
  %insert_new_size219 = insertvalue { i32, ptr } undef, i32 %new_size216, 0
  %insert_new_data_ptr220 = insertvalue { i32, ptr } %insert_new_size219, ptr %new_data_ptr218, 1
  %call.conclude_current221 = tail call { %Seq, %Seq } @conclude_current(%Seq %1, %Seq %2)
  %struct_field_0222 = extractvalue { %Seq, %Seq } %call.conclude_current221, 0
  %struct_field_1223 = extractvalue { %Seq, %Seq } %call.conclude_current221, 1
  br label %match.merge

match.body.1204:                                  ; preds = %find_one.exit707
  %heap_array227 = tail call ptr @malloc(i32 3)
  store i8 94, ptr %heap_array227, align 1
  %heap_array227.repack417 = getelementptr inbounds nuw i8, ptr %heap_array227, i64 1
  store i8 60, ptr %heap_array227.repack417, align 1
  %heap_array227.repack418 = getelementptr inbounds nuw i8, ptr %heap_array227, i64 2
  store i8 0, ptr %heap_array227.repack418, align 1
  %insert_array_data228 = insertvalue { i32, ptr } undef, ptr %heap_array227, 1
  %insert_array_size229 = insertvalue { i32, ptr } %insert_array_data228, i32 2, 0
  %call.record_member230 = tail call { i8, { i32, i32 } } @find_one({ i32, ptr } %0, { i32, ptr } %insert_array_size229)
  %tag239 = extractvalue { i8, { i32, i32 } } %call.record_member230, 0
  %eq_int240 = icmp eq i8 %tag239, 0
  br i1 %eq_int240, label %match.test.0234, label %match.body.1238

match.test.0234:                                  ; preds = %match.body.1204
  %extract_sum_type_payload241 = extractvalue { i8, { i32, i32 } } %call.record_member230, 1
  %struct_field_1245 = extractvalue { i32, i32 } %extract_sum_type_payload241, 1
  %new_size250 = sub i32 %get_array_size, %struct_field_1245
  %125 = sext i32 %struct_field_1245 to i64
  %new_data_ptr252 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %125
  %insert_new_size253 = insertvalue { i32, ptr } undef, i32 %new_size250, 0
  %insert_new_data_ptr254 = insertvalue { i32, ptr } %insert_new_size253, ptr %new_data_ptr252, 1
  %void_ptr255 = load ptr, ptr getelementptr inbounds nuw (i8, ptr @global_storage_array, i64 16), align 8
  %SEQ_ALT_TYPE_load = load i32, ptr %void_ptr255, align 4
  %126 = trunc i32 %SEQ_ALT_TYPE_load to i8
  %127 = insertvalue [16 x i8] poison, i8 %126, 0
  %128 = lshr i32 %SEQ_ALT_TYPE_load, 8
  %129 = trunc i32 %128 to i8
  %130 = insertvalue [16 x i8] %127, i8 %129, 1
  %131 = lshr i32 %SEQ_ALT_TYPE_load, 16
  %132 = trunc i32 %131 to i8
  %133 = insertvalue [16 x i8] %130, i8 %132, 2
  %134 = lshr i32 %SEQ_ALT_TYPE_load, 24
  %135 = trunc nuw i32 %134 to i8
  %136 = insertvalue [16 x i8] %133, i8 %135, 3
  %137 = insertvalue [16 x i8] %136, i8 0, 4
  %138 = insertvalue [16 x i8] %137, i8 0, 5
  %139 = insertvalue [16 x i8] %138, i8 0, 6
  %140 = insertvalue [16 x i8] %139, i8 0, 7
  %141 = insertvalue [16 x i8] %140, i8 0, 8
  %142 = insertvalue [16 x i8] %141, i8 0, 9
  %143 = insertvalue [16 x i8] %142, i8 0, 10
  %144 = insertvalue [16 x i8] %143, i8 0, 11
  %145 = insertvalue [16 x i8] %144, i8 0, 12
  %146 = insertvalue [16 x i8] %145, i8 0, 13
  %147 = insertvalue [16 x i8] %146, i8 0, 14
  %load_as_bytes260460 = insertvalue [16 x i8] %147, i8 0, 15
  %"insert variant data261" = insertvalue %Seq { i8 3, [16 x i8] undef }, [16 x i8] %load_as_bytes260460, 1
  %call.seq_extend.i708 = tail call %Seq @seq_extend(%Seq %2, %Seq %1)
  br label %match.merge

match.body.1238:                                  ; preds = %match.body.1204
  %heap_array263 = tail call ptr @malloc(i32 3)
  store i8 94, ptr %heap_array263, align 1
  %heap_array263.repack419 = getelementptr inbounds nuw i8, ptr %heap_array263, i64 1
  store i8 62, ptr %heap_array263.repack419, align 1
  %heap_array263.repack420 = getelementptr inbounds nuw i8, ptr %heap_array263, i64 2
  store i8 0, ptr %heap_array263.repack420, align 1
  %insert_array_data264 = insertvalue { i32, ptr } undef, ptr %heap_array263, 1
  %insert_array_size265 = insertvalue { i32, ptr } %insert_array_data264, i32 2, 0
  %call.record_member266 = tail call { i8, { i32, i32 } } @find_one({ i32, ptr } %0, { i32, ptr } %insert_array_size265)
  %tag275 = extractvalue { i8, { i32, i32 } } %call.record_member266, 0
  %eq_int276 = icmp eq i8 %tag275, 0
  br i1 %eq_int276, label %match.test.0270, label %match.body.1274

match.test.0270:                                  ; preds = %match.body.1238
  %extract_sum_type_payload277 = extractvalue { i8, { i32, i32 } } %call.record_member266, 1
  %struct_field_1281 = extractvalue { i32, i32 } %extract_sum_type_payload277, 1
  %new_size286 = sub i32 %get_array_size, %struct_field_1281
  %148 = sext i32 %struct_field_1281 to i64
  %new_data_ptr288 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %148
  %insert_new_size289 = insertvalue { i32, ptr } undef, i32 %new_size286, 0
  %insert_new_data_ptr290 = insertvalue { i32, ptr } %insert_new_size289, ptr %new_data_ptr288, 1
  %call.conclude_current291 = tail call { %Seq, %Seq } @conclude_current(%Seq %1, %Seq %2)
  %struct_field_0292 = extractvalue { %Seq, %Seq } %call.conclude_current291, 0
  %struct_field_1293 = extractvalue { %Seq, %Seq } %call.conclude_current291, 1
  br label %match.merge

match.body.1274:                                  ; preds = %match.body.1238
  %heap_array297 = tail call ptr @malloc(i32 6)
  store <4 x i8> <i8 94, i8 91, i8 32, i8 93>, ptr %heap_array297, align 1
  %heap_array297.repack424 = getelementptr inbounds nuw i8, ptr %heap_array297, i64 4
  store i8 43, ptr %heap_array297.repack424, align 1
  %heap_array297.repack425 = getelementptr inbounds nuw i8, ptr %heap_array297, i64 5
  store i8 0, ptr %heap_array297.repack425, align 1
  %insert_array_data298 = insertvalue { i32, ptr } undef, ptr %heap_array297, 1
  %insert_array_size299 = insertvalue { i32, ptr } %insert_array_data298, i32 5, 0
  %call.record_member300 = tail call { i8, { i32, i32 } } @find_one({ i32, ptr } %0, { i32, ptr } %insert_array_size299)
  %tag309 = extractvalue { i8, { i32, i32 } } %call.record_member300, 0
  %eq_int310 = icmp eq i8 %tag309, 0
  br i1 %eq_int310, label %match.test.0304, label %match.body.1308

match.test.0304:                                  ; preds = %match.body.1274
  %extract_sum_type_payload311 = extractvalue { i8, { i32, i32 } } %call.record_member300, 1
  %struct_field_1315 = extractvalue { i32, i32 } %extract_sum_type_payload311, 1
  %new_size320 = sub i32 %get_array_size, %struct_field_1315
  %149 = sext i32 %struct_field_1315 to i64
  %new_data_ptr322 = getelementptr i8, ptr %get_array_data_ptr.i, i64 %149
  %insert_new_size323 = insertvalue { i32, ptr } undef, i32 %new_size320, 0
  %insert_new_data_ptr324 = insertvalue { i32, ptr } %insert_new_size323, ptr %new_data_ptr322, 1
  br label %match.merge

match.body.1308:                                  ; preds = %match.body.1274
  %heap_array328 = tail call ptr @malloc(i32 1)
  store i8 0, ptr %heap_array328, align 1
  %insert_array_data329 = insertvalue { i32, ptr } undef, ptr %heap_array328, 1
  %insert_array_size330 = insertvalue { i32, ptr } %insert_array_data329, i32 0, 0
  br label %match.merge
}

define { i8, { i32, i32 } } @find_one({ i32, ptr } %0, { i32, ptr } %1) local_unnamed_addr #0 {
entry:
  %array_data_alloc = tail call ptr @malloc(i32 8)
  store i32 0, ptr %array_data_alloc, align 4
  %element_ptr1 = getelementptr i8, ptr %array_data_alloc, i64 4
  store i32 0, ptr %element_ptr1, align 4
  %get_array_data_ptr = extractvalue { i32, ptr } %0, 1
  %get_array_data_ptr2 = extractvalue { i32, ptr } %1, 1
  %call.regex_find_one = tail call i32 @regex_find_one(ptr %get_array_data_ptr, ptr %get_array_data_ptr2, ptr nonnull %array_data_alloc)
  %eq_int.not.not = icmp eq i32 %call.regex_find_one, 0
  br i1 %eq_int.not.not, label %match.body.1, label %common.ret

common.ret:                                       ; preds = %entry, %match.body.1
  %common.ret.op = phi { i8, { i32, i32 } } [ %"insert Some Value", %match.body.1 ], [ { i8 1, { i32, i32 } undef }, %entry ]
  ret { i8, { i32, i32 } } %common.ret.op

match.body.1:                                     ; preds = %entry
  %element = load i32, ptr %array_data_alloc, align 4
  %insertx = insertvalue { i32, i32 } undef, i32 %element, 0
  %element10 = load i32, ptr %element_ptr1, align 4
  %insertx11 = insertvalue { i32, i32 } %insertx, i32 %element10, 1
  %"insert Some Value" = insertvalue { i8, { i32, i32 } } { i8 0, { i32, i32 } undef }, { i32, i32 } %insertx11, 1
  br label %common.ret
}

declare i32 @regex_find_one(ptr %0, ptr %1, ptr %2) local_unnamed_addr #0

declare double @double_parse({ i32, ptr } %0) local_unnamed_addr #0

declare i32 @int32_parse({ i32, ptr } %0) local_unnamed_addr #0

define { { i32, ptr }, %Seq, %Seq } @start_new({ i32, ptr } %0, %Seq %1, %Seq %2, %Seq %3) local_unnamed_addr #0 {
entry:
  %call.seq_extend = tail call %Seq @seq_extend(%Seq %2, %Seq %1)
  %insertx = insertvalue { { i32, ptr }, %Seq, %Seq } undef, { i32, ptr } %0, 0
  %insertx1 = insertvalue { { i32, ptr }, %Seq, %Seq } %insertx, %Seq %3, 1
  %insertx2 = insertvalue { { i32, ptr }, %Seq, %Seq } %insertx1, %Seq %call.seq_extend, 2
  ret { { i32, ptr }, %Seq, %Seq } %insertx2
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(read, inaccessiblemem: none)
define %Seq @empty_seq() local_unnamed_addr #2 {
entry:
  %void_ptr = load ptr, ptr @global_storage_array, align 8
  %SEQ_LIST_TYPE_load = load i32, ptr %void_ptr, align 4
  %val_temp.sroa.0.0.extract.trunc = trunc i32 %SEQ_LIST_TYPE_load to i8
  %val_temp.sroa.2.0.extract.shift = lshr i32 %SEQ_LIST_TYPE_load, 8
  %val_temp.sroa.2.0.extract.trunc = trunc i32 %val_temp.sroa.2.0.extract.shift to i8
  %val_temp.sroa.3.0.extract.shift = lshr i32 %SEQ_LIST_TYPE_load, 16
  %val_temp.sroa.3.0.extract.trunc = trunc i32 %val_temp.sroa.3.0.extract.shift to i8
  %val_temp.sroa.4.0.extract.shift = lshr i32 %SEQ_LIST_TYPE_load, 24
  %val_temp.sroa.4.0.extract.trunc = trunc nuw i32 %val_temp.sroa.4.0.extract.shift to i8
  %load_as_bytes.fca.0.insert = insertvalue [16 x i8] poison, i8 %val_temp.sroa.0.0.extract.trunc, 0
  %load_as_bytes.fca.1.insert = insertvalue [16 x i8] %load_as_bytes.fca.0.insert, i8 %val_temp.sroa.2.0.extract.trunc, 1
  %load_as_bytes.fca.2.insert = insertvalue [16 x i8] %load_as_bytes.fca.1.insert, i8 %val_temp.sroa.3.0.extract.trunc, 2
  %load_as_bytes.fca.3.insert = insertvalue [16 x i8] %load_as_bytes.fca.2.insert, i8 %val_temp.sroa.4.0.extract.trunc, 3
  %load_as_bytes.fca.4.insert = insertvalue [16 x i8] %load_as_bytes.fca.3.insert, i8 0, 4
  %load_as_bytes.fca.5.insert = insertvalue [16 x i8] %load_as_bytes.fca.4.insert, i8 0, 5
  %load_as_bytes.fca.6.insert = insertvalue [16 x i8] %load_as_bytes.fca.5.insert, i8 0, 6
  %load_as_bytes.fca.7.insert = insertvalue [16 x i8] %load_as_bytes.fca.6.insert, i8 0, 7
  %load_as_bytes.fca.8.insert = insertvalue [16 x i8] %load_as_bytes.fca.7.insert, i8 0, 8
  %load_as_bytes.fca.9.insert = insertvalue [16 x i8] %load_as_bytes.fca.8.insert, i8 0, 9
  %load_as_bytes.fca.10.insert = insertvalue [16 x i8] %load_as_bytes.fca.9.insert, i8 0, 10
  %load_as_bytes.fca.11.insert = insertvalue [16 x i8] %load_as_bytes.fca.10.insert, i8 0, 11
  %load_as_bytes.fca.12.insert = insertvalue [16 x i8] %load_as_bytes.fca.11.insert, i8 0, 12
  %load_as_bytes.fca.13.insert = insertvalue [16 x i8] %load_as_bytes.fca.12.insert, i8 0, 13
  %load_as_bytes.fca.14.insert = insertvalue [16 x i8] %load_as_bytes.fca.13.insert, i8 0, 14
  %load_as_bytes.fca.15.insert = insertvalue [16 x i8] %load_as_bytes.fca.14.insert, i8 0, 15
  %"insert variant data" = insertvalue %Seq { i8 3, [16 x i8] undef }, [16 x i8] %load_as_bytes.fca.15.insert, 1
  ret %Seq %"insert variant data"
}

define %Seq @parse({ i32, ptr } %0) local_unnamed_addr #1 {
entry:
  %void_ptr.i = load ptr, ptr @global_storage_array, align 8
  %SEQ_LIST_TYPE_load.i = load i32, ptr %void_ptr.i, align 4
  %val_temp.sroa.0.0.extract.trunc.i = trunc i32 %SEQ_LIST_TYPE_load.i to i8
  %val_temp.sroa.2.0.extract.shift.i = lshr i32 %SEQ_LIST_TYPE_load.i, 8
  %val_temp.sroa.2.0.extract.trunc.i = trunc i32 %val_temp.sroa.2.0.extract.shift.i to i8
  %val_temp.sroa.3.0.extract.shift.i = lshr i32 %SEQ_LIST_TYPE_load.i, 16
  %val_temp.sroa.3.0.extract.trunc.i = trunc i32 %val_temp.sroa.3.0.extract.shift.i to i8
  %val_temp.sroa.4.0.extract.shift.i = lshr i32 %SEQ_LIST_TYPE_load.i, 24
  %val_temp.sroa.4.0.extract.trunc.i = trunc nuw i32 %val_temp.sroa.4.0.extract.shift.i to i8
  %load_as_bytes.fca.0.insert.i = insertvalue [16 x i8] poison, i8 %val_temp.sroa.0.0.extract.trunc.i, 0
  %load_as_bytes.fca.1.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.0.insert.i, i8 %val_temp.sroa.2.0.extract.trunc.i, 1
  %load_as_bytes.fca.2.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.1.insert.i, i8 %val_temp.sroa.3.0.extract.trunc.i, 2
  %load_as_bytes.fca.3.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.2.insert.i, i8 %val_temp.sroa.4.0.extract.trunc.i, 3
  %load_as_bytes.fca.4.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.3.insert.i, i8 0, 4
  %load_as_bytes.fca.5.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.4.insert.i, i8 0, 5
  %load_as_bytes.fca.6.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.5.insert.i, i8 0, 6
  %load_as_bytes.fca.7.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.6.insert.i, i8 0, 7
  %load_as_bytes.fca.8.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.7.insert.i, i8 0, 8
  %load_as_bytes.fca.9.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.8.insert.i, i8 0, 9
  %load_as_bytes.fca.10.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.9.insert.i, i8 0, 10
  %load_as_bytes.fca.11.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.10.insert.i, i8 0, 11
  %load_as_bytes.fca.12.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.11.insert.i, i8 0, 12
  %load_as_bytes.fca.13.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.12.insert.i, i8 0, 13
  %load_as_bytes.fca.14.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.13.insert.i, i8 0, 14
  %load_as_bytes.fca.15.insert.i = insertvalue [16 x i8] %load_as_bytes.fca.14.insert.i, i8 0, 15
  %"insert variant data.i" = insertvalue %Seq { i8 3, [16 x i8] undef }, [16 x i8] %load_as_bytes.fca.15.insert.i, 1
  %call.parse_seq = tail call %Seq @parse_seq({ i32, ptr } %0, %Seq %"insert variant data.i", %Seq %"insert variant data.i")
  %call.seq_reverse = tail call %Seq @seq_reverse(%Seq %call.parse_seq)
  ret %Seq %call.seq_reverse
}

define { i32, ptr } @ltype_str(i32 %0) local_unnamed_addr #0 {
entry:
  %heap_array = tail call ptr @malloc(i32 5)
  store <4 x i8> <i8 76, i8 105, i8 115, i8 116>, ptr %heap_array, align 1
  %heap_array.repack10 = getelementptr inbounds nuw i8, ptr %heap_array, i64 4
  store i8 0, ptr %heap_array.repack10, align 1
  %insert_array_data = insertvalue { i32, ptr } undef, ptr %heap_array, 1
  %insert_array_size = insertvalue { i32, ptr } %insert_array_data, i32 4, 0
  ret { i32, ptr } %insert_array_size
}

define void @print_seq_helper(i1 %0, %Seq %1) local_unnamed_addr #0 {
entry:
  %tag = extractvalue %Seq %1, 0
  switch i8 %tag, label %common.ret [
    i8 0, label %match.test.0
    i8 1, label %match.test.1
    i8 2, label %match.test.2
    i8 3, label %match.test.3
  ]

match.test.0:                                     ; preds = %entry
  %extract_sum_type_payload = extractvalue %Seq %1, 1
  %union_cast_temp = alloca [16 x i8], align 1
  %extract_sum_type_payload.elt = extractvalue [16 x i8] %extract_sum_type_payload, 0
  store i8 %extract_sum_type_payload.elt, ptr %union_cast_temp, align 1
  %union_cast_temp.repack307 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 1
  %extract_sum_type_payload.elt308 = extractvalue [16 x i8] %extract_sum_type_payload, 1
  store i8 %extract_sum_type_payload.elt308, ptr %union_cast_temp.repack307, align 1
  %union_cast_temp.repack309 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 2
  %extract_sum_type_payload.elt310 = extractvalue [16 x i8] %extract_sum_type_payload, 2
  store i8 %extract_sum_type_payload.elt310, ptr %union_cast_temp.repack309, align 1
  %union_cast_temp.repack311 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 3
  %extract_sum_type_payload.elt312 = extractvalue [16 x i8] %extract_sum_type_payload, 3
  store i8 %extract_sum_type_payload.elt312, ptr %union_cast_temp.repack311, align 1
  %union_to_scalar = load i32, ptr %union_cast_temp, align 4
  %str_buffer = alloca [20 x i8], align 1
  %2 = call i32 (ptr, ptr, ...) @sprintf(ptr nonnull dereferenceable(1) %str_buffer, ptr nonnull dereferenceable(1) @format_string.10, i32 %union_to_scalar)
  %strlen_call = call i32 @strlen(ptr nonnull %str_buffer)
  %printf_call = call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 %strlen_call, ptr nonnull %str_buffer)
  %heap_array = tail call ptr @malloc(i32 3)
  store i8 44, ptr %heap_array, align 1
  %heap_array.repack337 = getelementptr inbounds nuw i8, ptr %heap_array, i64 1
  store i8 32, ptr %heap_array.repack337, align 1
  %heap_array.repack338 = getelementptr inbounds nuw i8, ptr %heap_array, i64 2
  store i8 0, ptr %heap_array.repack338, align 1
  %printf_call5 = call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 2, ptr nonnull %heap_array)
  %fflush_stdout = call i32 @fflush(ptr null)
  br label %common.ret

common.ret:                                       ; preds = %entry, %match.merge, %match.body.3, %match.test.2, %match.test.1, %match.test.0
  ret void

match.test.1:                                     ; preds = %entry
  %extract_sum_type_payload8 = extractvalue %Seq %1, 1
  %union_cast_temp9 = alloca [16 x i8], align 1
  %extract_sum_type_payload8.elt = extractvalue [16 x i8] %extract_sum_type_payload8, 0
  store i8 %extract_sum_type_payload8.elt, ptr %union_cast_temp9, align 1
  %union_cast_temp9.repack275 = getelementptr inbounds nuw i8, ptr %union_cast_temp9, i64 1
  %extract_sum_type_payload8.elt276 = extractvalue [16 x i8] %extract_sum_type_payload8, 1
  store i8 %extract_sum_type_payload8.elt276, ptr %union_cast_temp9.repack275, align 1
  %union_cast_temp9.repack277 = getelementptr inbounds nuw i8, ptr %union_cast_temp9, i64 2
  %extract_sum_type_payload8.elt278 = extractvalue [16 x i8] %extract_sum_type_payload8, 2
  store i8 %extract_sum_type_payload8.elt278, ptr %union_cast_temp9.repack277, align 1
  %union_cast_temp9.repack279 = getelementptr inbounds nuw i8, ptr %union_cast_temp9, i64 3
  %extract_sum_type_payload8.elt280 = extractvalue [16 x i8] %extract_sum_type_payload8, 3
  store i8 %extract_sum_type_payload8.elt280, ptr %union_cast_temp9.repack279, align 1
  %union_cast_temp9.repack281 = getelementptr inbounds nuw i8, ptr %union_cast_temp9, i64 4
  %extract_sum_type_payload8.elt282 = extractvalue [16 x i8] %extract_sum_type_payload8, 4
  store i8 %extract_sum_type_payload8.elt282, ptr %union_cast_temp9.repack281, align 1
  %union_cast_temp9.repack283 = getelementptr inbounds nuw i8, ptr %union_cast_temp9, i64 5
  %extract_sum_type_payload8.elt284 = extractvalue [16 x i8] %extract_sum_type_payload8, 5
  store i8 %extract_sum_type_payload8.elt284, ptr %union_cast_temp9.repack283, align 1
  %union_cast_temp9.repack285 = getelementptr inbounds nuw i8, ptr %union_cast_temp9, i64 6
  %extract_sum_type_payload8.elt286 = extractvalue [16 x i8] %extract_sum_type_payload8, 6
  store i8 %extract_sum_type_payload8.elt286, ptr %union_cast_temp9.repack285, align 1
  %union_cast_temp9.repack287 = getelementptr inbounds nuw i8, ptr %union_cast_temp9, i64 7
  %extract_sum_type_payload8.elt288 = extractvalue [16 x i8] %extract_sum_type_payload8, 7
  store i8 %extract_sum_type_payload8.elt288, ptr %union_cast_temp9.repack287, align 1
  %union_to_scalar10 = load double, ptr %union_cast_temp9, align 8
  %str_buffer11 = alloca [20 x i8], align 1
  %3 = call i32 (ptr, ptr, ...) @sprintf(ptr nonnull dereferenceable(1) %str_buffer11, ptr nonnull dereferenceable(1) @format_string.2, double %union_to_scalar10)
  %4 = call i32 @strlen(ptr nonnull %str_buffer11)
  %printf_call16 = call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 %4, ptr nonnull %str_buffer11)
  %heap_array17 = tail call ptr @malloc(i32 3)
  store i8 44, ptr %heap_array17, align 1
  %heap_array17.repack305 = getelementptr inbounds nuw i8, ptr %heap_array17, i64 1
  store i8 32, ptr %heap_array17.repack305, align 1
  %heap_array17.repack306 = getelementptr inbounds nuw i8, ptr %heap_array17, i64 2
  store i8 0, ptr %heap_array17.repack306, align 1
  %printf_call22 = call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 2, ptr nonnull %heap_array17)
  %fflush_stdout23 = call i32 @fflush(ptr null)
  br label %common.ret

match.test.2:                                     ; preds = %entry
  %extract_sum_type_payload26 = extractvalue %Seq %1, 1
  %union_cast_temp27 = alloca [16 x i8], align 1
  %extract_sum_type_payload26.elt = extractvalue [16 x i8] %extract_sum_type_payload26, 0
  store i8 %extract_sum_type_payload26.elt, ptr %union_cast_temp27, align 1
  %union_cast_temp27.repack243 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 1
  %extract_sum_type_payload26.elt244 = extractvalue [16 x i8] %extract_sum_type_payload26, 1
  store i8 %extract_sum_type_payload26.elt244, ptr %union_cast_temp27.repack243, align 1
  %union_cast_temp27.repack245 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 2
  %extract_sum_type_payload26.elt246 = extractvalue [16 x i8] %extract_sum_type_payload26, 2
  store i8 %extract_sum_type_payload26.elt246, ptr %union_cast_temp27.repack245, align 1
  %union_cast_temp27.repack247 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 3
  %extract_sum_type_payload26.elt248 = extractvalue [16 x i8] %extract_sum_type_payload26, 3
  store i8 %extract_sum_type_payload26.elt248, ptr %union_cast_temp27.repack247, align 1
  %union_cast_temp27.repack249 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 4
  %extract_sum_type_payload26.elt250 = extractvalue [16 x i8] %extract_sum_type_payload26, 4
  store i8 %extract_sum_type_payload26.elt250, ptr %union_cast_temp27.repack249, align 1
  %union_cast_temp27.repack251 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 5
  %extract_sum_type_payload26.elt252 = extractvalue [16 x i8] %extract_sum_type_payload26, 5
  store i8 %extract_sum_type_payload26.elt252, ptr %union_cast_temp27.repack251, align 1
  %union_cast_temp27.repack253 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 6
  %extract_sum_type_payload26.elt254 = extractvalue [16 x i8] %extract_sum_type_payload26, 6
  store i8 %extract_sum_type_payload26.elt254, ptr %union_cast_temp27.repack253, align 1
  %union_cast_temp27.repack255 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 7
  %extract_sum_type_payload26.elt256 = extractvalue [16 x i8] %extract_sum_type_payload26, 7
  store i8 %extract_sum_type_payload26.elt256, ptr %union_cast_temp27.repack255, align 1
  %union_cast_temp27.repack257 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 8
  %extract_sum_type_payload26.elt258 = extractvalue [16 x i8] %extract_sum_type_payload26, 8
  store i8 %extract_sum_type_payload26.elt258, ptr %union_cast_temp27.repack257, align 1
  %union_cast_temp27.repack259 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 9
  %extract_sum_type_payload26.elt260 = extractvalue [16 x i8] %extract_sum_type_payload26, 9
  store i8 %extract_sum_type_payload26.elt260, ptr %union_cast_temp27.repack259, align 1
  %union_cast_temp27.repack261 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 10
  %extract_sum_type_payload26.elt262 = extractvalue [16 x i8] %extract_sum_type_payload26, 10
  store i8 %extract_sum_type_payload26.elt262, ptr %union_cast_temp27.repack261, align 1
  %union_cast_temp27.repack263 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 11
  %extract_sum_type_payload26.elt264 = extractvalue [16 x i8] %extract_sum_type_payload26, 11
  store i8 %extract_sum_type_payload26.elt264, ptr %union_cast_temp27.repack263, align 1
  %union_cast_temp27.repack265 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 12
  %extract_sum_type_payload26.elt266 = extractvalue [16 x i8] %extract_sum_type_payload26, 12
  store i8 %extract_sum_type_payload26.elt266, ptr %union_cast_temp27.repack265, align 1
  %union_cast_temp27.repack267 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 13
  %extract_sum_type_payload26.elt268 = extractvalue [16 x i8] %extract_sum_type_payload26, 13
  store i8 %extract_sum_type_payload26.elt268, ptr %union_cast_temp27.repack267, align 1
  %union_cast_temp27.repack269 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 14
  %extract_sum_type_payload26.elt270 = extractvalue [16 x i8] %extract_sum_type_payload26, 14
  store i8 %extract_sum_type_payload26.elt270, ptr %union_cast_temp27.repack269, align 1
  %union_cast_temp27.repack271 = getelementptr inbounds nuw i8, ptr %union_cast_temp27, i64 15
  %extract_sum_type_payload26.elt272 = extractvalue [16 x i8] %extract_sum_type_payload26, 15
  store i8 %extract_sum_type_payload26.elt272, ptr %union_cast_temp27.repack271, align 1
  %union_to_struct = load { i32, ptr }, ptr %union_cast_temp27, align 8
  %string_chars28 = extractvalue { i32, ptr } %union_to_struct, 1
  %string_len29 = extractvalue { i32, ptr } %union_to_struct, 0
  %printf_call30 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 %string_len29, ptr %string_chars28)
  %heap_array31 = tail call ptr @malloc(i32 3)
  store i8 44, ptr %heap_array31, align 1
  %heap_array31.repack273 = getelementptr inbounds nuw i8, ptr %heap_array31, i64 1
  store i8 32, ptr %heap_array31.repack273, align 1
  %heap_array31.repack274 = getelementptr inbounds nuw i8, ptr %heap_array31, i64 2
  store i8 0, ptr %heap_array31.repack274, align 1
  %printf_call36 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 2, ptr nonnull %heap_array31)
  %fflush_stdout37 = tail call i32 @fflush(ptr null)
  br label %common.ret

match.test.3:                                     ; preds = %entry
  %extract_sum_type_payload40 = extractvalue %Seq %1, 1
  %union_cast_temp41 = alloca [16 x i8], align 1
  %union_cast_temp41.repack100 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 4
  %extract_sum_type_payload40.elt101 = extractvalue [16 x i8] %extract_sum_type_payload40, 4
  store i8 %extract_sum_type_payload40.elt101, ptr %union_cast_temp41.repack100, align 1
  %union_cast_temp41.repack102 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 5
  %extract_sum_type_payload40.elt103 = extractvalue [16 x i8] %extract_sum_type_payload40, 5
  store i8 %extract_sum_type_payload40.elt103, ptr %union_cast_temp41.repack102, align 1
  %union_cast_temp41.repack104 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 6
  %extract_sum_type_payload40.elt105 = extractvalue [16 x i8] %extract_sum_type_payload40, 6
  store i8 %extract_sum_type_payload40.elt105, ptr %union_cast_temp41.repack104, align 1
  %union_cast_temp41.repack106 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 7
  %extract_sum_type_payload40.elt107 = extractvalue [16 x i8] %extract_sum_type_payload40, 7
  store i8 %extract_sum_type_payload40.elt107, ptr %union_cast_temp41.repack106, align 1
  %union_cast_temp41.repack108 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 8
  %extract_sum_type_payload40.elt109 = extractvalue [16 x i8] %extract_sum_type_payload40, 8
  store i8 %extract_sum_type_payload40.elt109, ptr %union_cast_temp41.repack108, align 1
  %union_cast_temp41.repack110 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 9
  %extract_sum_type_payload40.elt111 = extractvalue [16 x i8] %extract_sum_type_payload40, 9
  store i8 %extract_sum_type_payload40.elt111, ptr %union_cast_temp41.repack110, align 1
  %union_cast_temp41.repack112 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 10
  %extract_sum_type_payload40.elt113 = extractvalue [16 x i8] %extract_sum_type_payload40, 10
  store i8 %extract_sum_type_payload40.elt113, ptr %union_cast_temp41.repack112, align 1
  %union_cast_temp41.repack114 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 11
  %extract_sum_type_payload40.elt115 = extractvalue [16 x i8] %extract_sum_type_payload40, 11
  store i8 %extract_sum_type_payload40.elt115, ptr %union_cast_temp41.repack114, align 1
  %union_cast_temp41.repack116 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 12
  %extract_sum_type_payload40.elt117 = extractvalue [16 x i8] %extract_sum_type_payload40, 12
  store i8 %extract_sum_type_payload40.elt117, ptr %union_cast_temp41.repack116, align 1
  %union_cast_temp41.repack118 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 13
  %extract_sum_type_payload40.elt119 = extractvalue [16 x i8] %extract_sum_type_payload40, 13
  store i8 %extract_sum_type_payload40.elt119, ptr %union_cast_temp41.repack118, align 1
  %union_cast_temp41.repack120 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 14
  %extract_sum_type_payload40.elt121 = extractvalue [16 x i8] %extract_sum_type_payload40, 14
  store i8 %extract_sum_type_payload40.elt121, ptr %union_cast_temp41.repack120, align 1
  %union_cast_temp41.repack122 = getelementptr inbounds nuw i8, ptr %union_cast_temp41, i64 15
  %extract_sum_type_payload40.elt123 = extractvalue [16 x i8] %extract_sum_type_payload40, 15
  store i8 %extract_sum_type_payload40.elt123, ptr %union_cast_temp41.repack122, align 1
  %union_to_struct42.unpack125 = load i32, ptr %union_cast_temp41.repack100, align 4
  %union_to_struct42.unpack127 = load ptr, ptr %union_cast_temp41.repack108, align 8
  %eq_int43 = icmp eq i32 %union_to_struct42.unpack125, 0
  %is_null = icmp eq ptr %union_to_struct42.unpack127, null
  %match_succ_accumulator44 = and i1 %eq_int43, %is_null
  br i1 %match_succ_accumulator44, label %match.body.3, label %match.test.4

match.body.3:                                     ; preds = %match.test.3
  %heap_array45 = tail call ptr @malloc(i32 4)
  store <4 x i8> <i8 93, i8 44, i8 32, i8 0>, ptr %heap_array45, align 1
  %printf_call50 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 3, ptr nonnull %heap_array45)
  %fflush_stdout51 = tail call i32 @fflush(ptr null)
  br label %common.ret

match.test.4:                                     ; preds = %match.test.3
  %extract_sum_type_payload40.elt99 = extractvalue [16 x i8] %extract_sum_type_payload40, 3
  %extract_sum_type_payload40.elt97 = extractvalue [16 x i8] %extract_sum_type_payload40, 2
  %extract_sum_type_payload40.elt95 = extractvalue [16 x i8] %extract_sum_type_payload40, 1
  %extract_sum_type_payload40.elt = extractvalue [16 x i8] %extract_sum_type_payload40, 0
  %union_cast_temp55 = alloca [16 x i8], align 1
  store i8 %extract_sum_type_payload40.elt, ptr %union_cast_temp55, align 1
  %union_cast_temp55.repack130 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 1
  store i8 %extract_sum_type_payload40.elt95, ptr %union_cast_temp55.repack130, align 1
  %union_cast_temp55.repack132 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 2
  store i8 %extract_sum_type_payload40.elt97, ptr %union_cast_temp55.repack132, align 1
  %union_cast_temp55.repack134 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 3
  store i8 %extract_sum_type_payload40.elt99, ptr %union_cast_temp55.repack134, align 1
  %union_cast_temp55.repack136 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 4
  store i8 %extract_sum_type_payload40.elt101, ptr %union_cast_temp55.repack136, align 1
  %union_cast_temp55.repack138 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 5
  store i8 %extract_sum_type_payload40.elt103, ptr %union_cast_temp55.repack138, align 1
  %union_cast_temp55.repack140 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 6
  store i8 %extract_sum_type_payload40.elt105, ptr %union_cast_temp55.repack140, align 1
  %union_cast_temp55.repack142 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 7
  store i8 %extract_sum_type_payload40.elt107, ptr %union_cast_temp55.repack142, align 1
  %union_cast_temp55.repack144 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 8
  store i8 %extract_sum_type_payload40.elt109, ptr %union_cast_temp55.repack144, align 1
  %union_cast_temp55.repack146 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 9
  store i8 %extract_sum_type_payload40.elt111, ptr %union_cast_temp55.repack146, align 1
  %union_cast_temp55.repack148 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 10
  store i8 %extract_sum_type_payload40.elt113, ptr %union_cast_temp55.repack148, align 1
  %union_cast_temp55.repack150 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 11
  store i8 %extract_sum_type_payload40.elt115, ptr %union_cast_temp55.repack150, align 1
  %union_cast_temp55.repack152 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 12
  store i8 %extract_sum_type_payload40.elt117, ptr %union_cast_temp55.repack152, align 1
  %union_cast_temp55.repack154 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 13
  store i8 %extract_sum_type_payload40.elt119, ptr %union_cast_temp55.repack154, align 1
  %union_cast_temp55.repack156 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 14
  store i8 %extract_sum_type_payload40.elt121, ptr %union_cast_temp55.repack156, align 1
  %union_cast_temp55.repack158 = getelementptr inbounds nuw i8, ptr %union_cast_temp55, i64 15
  store i8 %extract_sum_type_payload40.elt123, ptr %union_cast_temp55.repack158, align 1
  %union_to_struct56.unpack = load i32, ptr %union_cast_temp55, align 8
  %union_to_struct56.unpack161 = load i32, ptr %union_cast_temp55.repack136, align 4
  %union_to_struct56.unpack163 = load ptr, ptr %union_cast_temp55.repack144, align 8
  %is_null60 = icmp eq ptr %union_to_struct56.unpack163, null
  br i1 %is_null60, label %match.test.062, label %list_cons_test_elements

list_cons_test_elements:                          ; preds = %match.test.4
  %list_head.unpack = load i8, ptr %union_to_struct56.unpack163, align 1
  %5 = insertvalue %Seq poison, i8 %list_head.unpack, 0
  %list_head.elt165 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 1
  %list_head.unpack166.unpack = load i8, ptr %list_head.elt165, align 1
  %6 = insertvalue [16 x i8] poison, i8 %list_head.unpack166.unpack, 0
  %list_head.unpack166.elt168 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 2
  %list_head.unpack166.unpack169 = load i8, ptr %list_head.unpack166.elt168, align 1
  %7 = insertvalue [16 x i8] %6, i8 %list_head.unpack166.unpack169, 1
  %list_head.unpack166.elt170 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 3
  %list_head.unpack166.unpack171 = load i8, ptr %list_head.unpack166.elt170, align 1
  %8 = insertvalue [16 x i8] %7, i8 %list_head.unpack166.unpack171, 2
  %list_head.unpack166.elt172 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 4
  %list_head.unpack166.unpack173 = load i8, ptr %list_head.unpack166.elt172, align 1
  %9 = insertvalue [16 x i8] %8, i8 %list_head.unpack166.unpack173, 3
  %list_head.unpack166.elt174 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 5
  %list_head.unpack166.unpack175 = load i8, ptr %list_head.unpack166.elt174, align 1
  %10 = insertvalue [16 x i8] %9, i8 %list_head.unpack166.unpack175, 4
  %list_head.unpack166.elt176 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 6
  %list_head.unpack166.unpack177 = load i8, ptr %list_head.unpack166.elt176, align 1
  %11 = insertvalue [16 x i8] %10, i8 %list_head.unpack166.unpack177, 5
  %list_head.unpack166.elt178 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 7
  %list_head.unpack166.unpack179 = load i8, ptr %list_head.unpack166.elt178, align 1
  %12 = insertvalue [16 x i8] %11, i8 %list_head.unpack166.unpack179, 6
  %list_head.unpack166.elt180 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 8
  %list_head.unpack166.unpack181 = load i8, ptr %list_head.unpack166.elt180, align 1
  %13 = insertvalue [16 x i8] %12, i8 %list_head.unpack166.unpack181, 7
  %list_head.unpack166.elt182 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 9
  %list_head.unpack166.unpack183 = load i8, ptr %list_head.unpack166.elt182, align 1
  %14 = insertvalue [16 x i8] %13, i8 %list_head.unpack166.unpack183, 8
  %list_head.unpack166.elt184 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 10
  %list_head.unpack166.unpack185 = load i8, ptr %list_head.unpack166.elt184, align 1
  %15 = insertvalue [16 x i8] %14, i8 %list_head.unpack166.unpack185, 9
  %list_head.unpack166.elt186 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 11
  %list_head.unpack166.unpack187 = load i8, ptr %list_head.unpack166.elt186, align 1
  %16 = insertvalue [16 x i8] %15, i8 %list_head.unpack166.unpack187, 10
  %list_head.unpack166.elt188 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 12
  %list_head.unpack166.unpack189 = load i8, ptr %list_head.unpack166.elt188, align 1
  %17 = insertvalue [16 x i8] %16, i8 %list_head.unpack166.unpack189, 11
  %list_head.unpack166.elt190 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 13
  %list_head.unpack166.unpack191 = load i8, ptr %list_head.unpack166.elt190, align 1
  %18 = insertvalue [16 x i8] %17, i8 %list_head.unpack166.unpack191, 12
  %list_head.unpack166.elt192 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 14
  %list_head.unpack166.unpack193 = load i8, ptr %list_head.unpack166.elt192, align 1
  %19 = insertvalue [16 x i8] %18, i8 %list_head.unpack166.unpack193, 13
  %list_head.unpack166.elt194 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 15
  %list_head.unpack166.unpack195 = load i8, ptr %list_head.unpack166.elt194, align 1
  %20 = insertvalue [16 x i8] %19, i8 %list_head.unpack166.unpack195, 14
  %list_head.unpack166.elt196 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 16
  %list_head.unpack166.unpack197 = load i8, ptr %list_head.unpack166.elt196, align 1
  %list_head.unpack166198 = insertvalue [16 x i8] %20, i8 %list_head.unpack166.unpack197, 15
  %list_head167 = insertvalue %Seq %5, [16 x i8] %list_head.unpack166198, 1
  %item_ptr61 = getelementptr inbounds nuw i8, ptr %union_to_struct56.unpack163, i64 24
  %list_rest = load ptr, ptr %item_ptr61, align 8
  %21 = ptrtoint ptr %list_rest to i64
  %extract.t = trunc i64 %21 to i8
  %extract = lshr i64 %21, 8
  %extract.t339 = trunc i64 %extract to i8
  %extract340 = lshr i64 %21, 16
  %extract.t341 = trunc i64 %extract340 to i8
  %extract342 = lshr i64 %21, 24
  %extract.t343 = trunc i64 %extract342 to i8
  %extract344 = lshr i64 %21, 32
  %extract.t345 = trunc i64 %extract344 to i8
  %extract346 = lshr i64 %21, 40
  %extract.t347 = trunc i64 %extract346 to i8
  %extract348 = lshr i64 %21, 48
  %extract.t349 = trunc i64 %extract348 to i8
  %extract350 = lshr i64 %21, 56
  %extract.t351 = trunc nuw i64 %extract350 to i8
  br label %match.test.062

match.merge:                                      ; preds = %match.test.062, %match.body.063
  %call.print_seq_helper = call void @print_seq_helper(i1 true, %Seq %head_val)
  %-_int = add i32 %union_to_struct56.unpack161, -1
  %22 = trunc i32 %union_to_struct56.unpack to i8
  %23 = insertvalue [16 x i8] poison, i8 %22, 0
  %24 = lshr i32 %union_to_struct56.unpack, 8
  %25 = trunc i32 %24 to i8
  %26 = insertvalue [16 x i8] %23, i8 %25, 1
  %27 = lshr i32 %union_to_struct56.unpack, 16
  %28 = trunc i32 %27 to i8
  %29 = insertvalue [16 x i8] %26, i8 %28, 2
  %30 = lshr i32 %union_to_struct56.unpack, 24
  %31 = trunc nuw i32 %30 to i8
  %32 = insertvalue [16 x i8] %29, i8 %31, 3
  %33 = trunc i32 %-_int to i8
  %34 = insertvalue [16 x i8] %32, i8 %33, 4
  %35 = lshr i32 %-_int, 8
  %36 = trunc i32 %35 to i8
  %37 = insertvalue [16 x i8] %34, i8 %36, 5
  %38 = lshr i32 %-_int, 16
  %39 = trunc i32 %38 to i8
  %40 = insertvalue [16 x i8] %37, i8 %39, 6
  %41 = lshr i32 %-_int, 24
  %42 = trunc nuw i32 %41 to i8
  %43 = insertvalue [16 x i8] %40, i8 %42, 7
  %44 = insertvalue [16 x i8] %43, i8 %tail_val.off0, 8
  %45 = insertvalue [16 x i8] %44, i8 %tail_val.off8, 9
  %46 = insertvalue [16 x i8] %45, i8 %tail_val.off16, 10
  %47 = insertvalue [16 x i8] %46, i8 %tail_val.off24, 11
  %48 = insertvalue [16 x i8] %47, i8 %tail_val.off32, 12
  %49 = insertvalue [16 x i8] %48, i8 %tail_val.off40, 13
  %50 = insertvalue [16 x i8] %49, i8 %tail_val.off48, 14
  %load_as_bytes239 = insertvalue [16 x i8] %50, i8 %tail_val.off56, 15
  %"insert variant data" = insertvalue %Seq { i8 3, [16 x i8] undef }, [16 x i8] %load_as_bytes239, 1
  %call.print_seq_helper93 = tail call void @print_seq_helper(i1 false, %Seq %"insert variant data")
  br label %common.ret

match.test.062:                                   ; preds = %match.test.4, %list_cons_test_elements
  %head_val = phi %Seq [ undef, %match.test.4 ], [ %list_head167, %list_cons_test_elements ]
  %tail_val.off0 = phi i8 [ undef, %match.test.4 ], [ %extract.t, %list_cons_test_elements ]
  %tail_val.off8 = phi i8 [ 0, %match.test.4 ], [ %extract.t339, %list_cons_test_elements ]
  %tail_val.off16 = phi i8 [ 0, %match.test.4 ], [ %extract.t341, %list_cons_test_elements ]
  %tail_val.off24 = phi i8 [ 0, %match.test.4 ], [ %extract.t343, %list_cons_test_elements ]
  %tail_val.off32 = phi i8 [ 0, %match.test.4 ], [ %extract.t345, %list_cons_test_elements ]
  %tail_val.off40 = phi i8 [ 0, %match.test.4 ], [ %extract.t347, %list_cons_test_elements ]
  %tail_val.off48 = phi i8 [ 0, %match.test.4 ], [ %extract.t349, %list_cons_test_elements ]
  %tail_val.off56 = phi i8 [ 0, %match.test.4 ], [ %extract.t351, %list_cons_test_elements ]
  br i1 %0, label %match.body.063, label %match.merge

match.body.063:                                   ; preds = %match.test.062
  %heap_array.i = tail call ptr @malloc(i32 5)
  store <4 x i8> <i8 76, i8 105, i8 115, i8 116>, ptr %heap_array.i, align 1
  %heap_array.repack10.i = getelementptr inbounds nuw i8, ptr %heap_array.i, i64 4
  store i8 0, ptr %heap_array.repack10.i, align 1
  %printf_call69 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 4, ptr nonnull %heap_array.i)
  %heap_array70 = tail call ptr @malloc(i32 4)
  store <4 x i8> <i8 58, i8 40, i8 35, i8 0>, ptr %heap_array70, align 1
  %printf_call75 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 3, ptr nonnull %heap_array70)
  %str_buffer76 = alloca [20 x i8], align 1
  %51 = call i32 (ptr, ptr, ...) @sprintf(ptr nonnull dereferenceable(1) %str_buffer76, ptr nonnull dereferenceable(1) @format_string.10, i32 %union_to_struct56.unpack161)
  %strlen_call77 = call i32 @strlen(ptr nonnull %str_buffer76)
  %printf_call82 = call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 %strlen_call77, ptr nonnull %str_buffer76)
  %heap_array83 = tail call ptr @malloc(i32 4)
  store <4 x i8> <i8 41, i8 91, i8 32, i8 0>, ptr %heap_array83, align 1
  %printf_call88 = call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 3, ptr nonnull %heap_array83)
  %fflush_stdout89 = call i32 @fflush(ptr null)
  br label %match.merge
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none) %0, ...) local_unnamed_addr #3

; Function Attrs: nofree nounwind
declare noundef i32 @fflush(ptr noundef captures(none) %0) local_unnamed_addr #3

; Function Attrs: nofree nounwind
declare noundef i32 @sprintf(ptr noalias noundef writeonly captures(none) %0, ptr noundef readonly captures(none) %1, ...) local_unnamed_addr #3

declare i32 @strlen(ptr %0) local_unnamed_addr #0

define noalias noundef nonnull ptr @coro_dummy() local_unnamed_addr #0 {
AfterCoroEnd:
  %coro.frame = tail call ptr @malloc(i32 40)
  store ptr @coro_dummy.resume, ptr %coro.frame, align 8
  %destroy.addr = getelementptr inbounds nuw i8, ptr %coro.frame, i64 8
  store ptr @coro_dummy.destroy, ptr %destroy.addr, align 8
  %.repack89 = getelementptr inbounds nuw i8, ptr %coro.frame, i64 33
  store i1 false, ptr %.repack89, align 1
  %index.addr87 = getelementptr inbounds nuw i8, ptr %coro.frame, i64 34
  store i2 0, ptr %index.addr87, align 1
  ret ptr %coro.frame
}

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr noundef captures(none) %0) local_unnamed_addr #4

define noalias noundef nonnull ptr @pat_to_coroutine(%Seq %0) local_unnamed_addr #0 {
entry:
  %tag = extractvalue %Seq %0, 0
  %eq_int = icmp eq i8 %tag, 3
  br i1 %eq_int, label %match.test.0, label %match.body.1

match.test.0:                                     ; preds = %entry
  %extract_sum_type_payload = extractvalue %Seq %0, 1
  %union_cast_temp = alloca [16 x i8], align 1
  %extract_sum_type_payload.elt = extractvalue [16 x i8] %extract_sum_type_payload, 0
  store i8 %extract_sum_type_payload.elt, ptr %union_cast_temp, align 1
  %union_cast_temp.repack4 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 1
  %extract_sum_type_payload.elt5 = extractvalue [16 x i8] %extract_sum_type_payload, 1
  store i8 %extract_sum_type_payload.elt5, ptr %union_cast_temp.repack4, align 1
  %union_cast_temp.repack6 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 2
  %extract_sum_type_payload.elt7 = extractvalue [16 x i8] %extract_sum_type_payload, 2
  store i8 %extract_sum_type_payload.elt7, ptr %union_cast_temp.repack6, align 1
  %union_cast_temp.repack8 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 3
  %extract_sum_type_payload.elt9 = extractvalue [16 x i8] %extract_sum_type_payload, 3
  store i8 %extract_sum_type_payload.elt9, ptr %union_cast_temp.repack8, align 1
  %union_cast_temp.repack18 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 8
  %extract_sum_type_payload.elt19 = extractvalue [16 x i8] %extract_sum_type_payload, 8
  store i8 %extract_sum_type_payload.elt19, ptr %union_cast_temp.repack18, align 1
  %union_cast_temp.repack20 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 9
  %extract_sum_type_payload.elt21 = extractvalue [16 x i8] %extract_sum_type_payload, 9
  store i8 %extract_sum_type_payload.elt21, ptr %union_cast_temp.repack20, align 1
  %union_cast_temp.repack22 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 10
  %extract_sum_type_payload.elt23 = extractvalue [16 x i8] %extract_sum_type_payload, 10
  store i8 %extract_sum_type_payload.elt23, ptr %union_cast_temp.repack22, align 1
  %union_cast_temp.repack24 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 11
  %extract_sum_type_payload.elt25 = extractvalue [16 x i8] %extract_sum_type_payload, 11
  store i8 %extract_sum_type_payload.elt25, ptr %union_cast_temp.repack24, align 1
  %union_cast_temp.repack26 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 12
  %extract_sum_type_payload.elt27 = extractvalue [16 x i8] %extract_sum_type_payload, 12
  store i8 %extract_sum_type_payload.elt27, ptr %union_cast_temp.repack26, align 1
  %union_cast_temp.repack28 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 13
  %extract_sum_type_payload.elt29 = extractvalue [16 x i8] %extract_sum_type_payload, 13
  store i8 %extract_sum_type_payload.elt29, ptr %union_cast_temp.repack28, align 1
  %union_cast_temp.repack30 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 14
  %extract_sum_type_payload.elt31 = extractvalue [16 x i8] %extract_sum_type_payload, 14
  store i8 %extract_sum_type_payload.elt31, ptr %union_cast_temp.repack30, align 1
  %union_cast_temp.repack32 = getelementptr inbounds nuw i8, ptr %union_cast_temp, i64 15
  %extract_sum_type_payload.elt33 = extractvalue [16 x i8] %extract_sum_type_payload, 15
  store i8 %extract_sum_type_payload.elt33, ptr %union_cast_temp.repack32, align 1
  %union_to_struct.unpack = load i32, ptr %union_cast_temp, align 8
  %eq_int1 = icmp eq i32 %union_to_struct.unpack, 3
  br i1 %eq_int1, label %match.body.0, label %match.body.1

common.ret:                                       ; preds = %match.body.1, %match.body.0
  %common.ret.op = phi ptr [ %coro.frame.i40, %match.body.0 ], [ %coro.frame.i44, %match.body.1 ]
  ret ptr %common.ret.op

match.body.0:                                     ; preds = %match.test.0
  %coro.frame.i = tail call ptr @malloc(i32 56)
  %coro.frame.i40 = tail call ptr @malloc(i32 40)
  store ptr @coro_loop_wrapper_0.resume, ptr %coro.frame.i40, align 8
  %destroy.addr.i42 = getelementptr inbounds nuw i8, ptr %coro.frame.i40, i64 8
  store ptr @coro_loop_wrapper_0.destroy, ptr %destroy.addr.i42, align 8
  %.repack68.i = getelementptr inbounds nuw i8, ptr %coro.frame.i40, i64 33
  store i1 false, ptr %.repack68.i, align 1
  %index.addr67.i = getelementptr inbounds nuw i8, ptr %coro.frame.i40, i64 34
  store i1 false, ptr %index.addr67.i, align 1
  br label %common.ret

match.body.1:                                     ; preds = %entry, %match.test.0
  %coro.frame.i44 = tail call ptr @malloc(i32 40)
  store ptr @coro_dummy.resume, ptr %coro.frame.i44, align 8
  %destroy.addr.i46 = getelementptr inbounds nuw i8, ptr %coro.frame.i44, i64 8
  store ptr @coro_dummy.destroy, ptr %destroy.addr.i46, align 8
  %.repack89.i = getelementptr inbounds nuw i8, ptr %coro.frame.i44, i64 33
  store i1 false, ptr %.repack89.i, align 1
  %index.addr87.i = getelementptr inbounds nuw i8, ptr %coro.frame.i44, i64 34
  store i2 0, ptr %index.addr87.i, align 1
  br label %common.ret
}

define noalias noundef nonnull ptr @coro_of_list_0(ptr %list.param) local_unnamed_addr #0 {
AfterCoroEnd:
  %coro.frame = tail call ptr @malloc(i32 56)
  store ptr @coro_of_list_0.resume, ptr %coro.frame, align 8
  %destroy.addr = getelementptr inbounds nuw i8, ptr %coro.frame, i64 8
  store ptr @coro_of_list_0.destroy, ptr %destroy.addr, align 8
  %.repack92 = getelementptr inbounds nuw i8, ptr %coro.frame, i64 33
  store i1 false, ptr %.repack92, align 1
  %list.param.spill.addr = getelementptr inbounds nuw i8, ptr %coro.frame, i64 40
  store ptr %list.param, ptr %list.param.spill.addr, align 8
  %index.addr90 = getelementptr inbounds nuw i8, ptr %coro.frame, i64 34
  store i2 0, ptr %index.addr90, align 1
  ret ptr %coro.frame
}

define noalias noundef nonnull ptr @coro_loop_wrapper_0(ptr readnone captures(none) %inner_coro.param) local_unnamed_addr #0 {
AfterCoroEnd:
  %coro.frame = tail call ptr @malloc(i32 40)
  store ptr @coro_loop_wrapper_0.resume, ptr %coro.frame, align 8
  %destroy.addr = getelementptr inbounds nuw i8, ptr %coro.frame, i64 8
  store ptr @coro_loop_wrapper_0.destroy, ptr %destroy.addr, align 8
  %.repack68 = getelementptr inbounds nuw i8, ptr %coro.frame, i64 33
  store i1 false, ptr %.repack68, align 1
  %index.addr67 = getelementptr inbounds nuw i8, ptr %coro.frame, i64 34
  store i1 false, ptr %index.addr67, align 1
  ret ptr %coro.frame
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.assume(i1 noundef %0) #5

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(read, argmem: readwrite, inaccessiblemem: none)
define internal fastcc void @coro_of_list_0.resume(ptr noundef nonnull align 8 captures(none) dereferenceable(56) %coro.handle) #6 {
resume.entry:
  %index.addr = getelementptr inbounds nuw i8, ptr %coro.handle, i64 34
  %index = load i2, ptr %index.addr, align 2
  %switch = icmp eq i2 %index, 0
  %next.ptr.reload.addr = getelementptr inbounds nuw i8, ptr %coro.handle, i64 48
  %next.ptr.reload = load ptr, ptr %next.ptr.reload.addr, align 8
  %list.param.reload.addr = getelementptr inbounds nuw i8, ptr %coro.handle, i64 40
  %list.param.reload = load ptr, ptr %list.param.reload.addr, align 8
  %current.0 = select i1 %switch, ptr %list.param.reload, ptr %next.ptr.reload
  %is.null = icmp eq ptr %current.0, null
  br i1 %is.null, label %AfterCoroSuspend73, label %AfterCoroSuspend70.thread

CoroEnd:                                          ; preds = %AfterCoroSuspend73, %AfterCoroSuspend70.thread
  ret void

AfterCoroSuspend70.thread:                        ; preds = %resume.entry
  %promise.reload.addr = getelementptr inbounds nuw i8, ptr %coro.handle, i64 16
  %promise.addr.repack36.repack6689 = getelementptr inbounds nuw i8, ptr %coro.handle, i64 32
  %node.data.unpack3.elt33 = getelementptr inbounds nuw i8, ptr %current.0, i64 16
  %node.data.unpack3.unpack34 = load i8, ptr %node.data.unpack3.elt33, align 1
  %0 = load <16 x i8>, ptr %current.0, align 1
  store <16 x i8> %0, ptr %promise.reload.addr, align 8
  store i8 %node.data.unpack3.unpack34, ptr %promise.addr.repack36.repack6689, align 8
  %next.ptr.ptr = getelementptr inbounds nuw i8, ptr %current.0, i64 24
  %next.ptr = load ptr, ptr %next.ptr.ptr, align 8
  store ptr %next.ptr, ptr %next.ptr.reload.addr, align 8
  store i2 1, ptr %index.addr, align 2
  br label %CoroEnd

AfterCoroSuspend73:                               ; preds = %resume.entry
  store ptr null, ptr %coro.handle, align 8
  br label %CoroEnd
}

; Function Attrs: mustprogress nounwind willreturn memory(argmem: readwrite, inaccessiblemem: readwrite)
define internal fastcc void @coro_of_list_0.destroy(ptr noundef nonnull align 8 captures(none) dereferenceable(56) %coro.handle) #7 {
resume.entry:
  tail call void @free(ptr nonnull %coro.handle)
  ret void
}

; Function Attrs: nofree norecurse noreturn nosync nounwind memory(none)
define internal fastcc void @coro_loop_wrapper_0.resume(ptr nonnull readnone align 8 captures(none) %coro.handle) #8 {
AfterCoroSuspend:
  br label %yield_from.exit

yield_from.exit:                                  ; preds = %AfterCoroSuspend, %yield_from.exit
  br label %yield_from.exit
}

; Function Attrs: mustprogress nounwind willreturn memory(argmem: readwrite, inaccessiblemem: readwrite)
define internal fastcc void @coro_loop_wrapper_0.destroy(ptr noundef nonnull align 8 captures(none) dereferenceable(40) %coro.handle) #7 {
CoroEnd:
  tail call void @free(ptr nonnull %coro.handle)
  ret void
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr writeonly captures(none) %0, i8 %1, i64 %2, i1 immarg %3) #9

define internal fastcc void @coro_dummy.resume(ptr noundef nonnull align 8 captures(none) dereferenceable(40) %coro.handle) #0 {
resume.entry:
  %index.addr = getelementptr inbounds nuw i8, ptr %coro.handle, i64 34
  %index = load i2, ptr %index.addr, align 2
  %switch = icmp eq i2 %index, 0
  br i1 %switch, label %AfterCoroSuspend83.thread, label %AfterCoroSuspend86

CoroEnd:                                          ; preds = %AfterCoroSuspend86, %AfterCoroSuspend83.thread
  ret void

AfterCoroSuspend83.thread:                        ; preds = %resume.entry
  %promise.reload.addr = getelementptr inbounds nuw i8, ptr %coro.handle, i64 16
  %heap_array = tail call ptr @malloc(i32 18)
  store <16 x i8> <i8 121, i8 105, i8 101, i8 108, i8 100, i8 32, i8 102, i8 114, i8 111, i8 109, i8 32, i8 100, i8 117, i8 109, i8 109, i8 121>, ptr %heap_array, align 1
  %heap_array.repack16 = getelementptr inbounds nuw i8, ptr %heap_array, i64 16
  store i8 10, ptr %heap_array.repack16, align 1
  %heap_array.repack17 = getelementptr inbounds nuw i8, ptr %heap_array, i64 17
  store i8 0, ptr %heap_array.repack17, align 1
  %printf_call = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @fmt_str.13, i32 17, ptr nonnull %heap_array)
  %fflush_stdout = tail call i32 @fflush(ptr null)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(5) %promise.reload.addr, i8 0, i64 5, i1 false)
  store i2 1, ptr %index.addr, align 2
  br label %CoroEnd

AfterCoroSuspend86:                               ; preds = %resume.entry
  store ptr null, ptr %coro.handle, align 8
  br label %CoroEnd
}

; Function Attrs: mustprogress nounwind willreturn memory(argmem: readwrite, inaccessiblemem: readwrite)
define internal fastcc void @coro_dummy.destroy(ptr noundef nonnull align 8 captures(none) dereferenceable(40) %coro.handle) #7 {
resume.entry:
  tail call void @free(ptr nonnull %coro.handle)
  ret void
}

attributes #0 = { "frame-pointer"="none" }
attributes #1 = { "frame-pointer"="none" }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(read, inaccessiblemem: none) "frame-pointer"="none" }
attributes #3 = { nofree nounwind "frame-pointer"="none" }
attributes #4 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" "frame-pointer"="none" }
attributes #5 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) "frame-pointer"="none" }
attributes #6 = { mustprogress nofree norecurse nosync nounwind willreturn memory(read, argmem: readwrite, inaccessiblemem: none) "frame-pointer"="none" }
attributes #7 = { mustprogress nounwind willreturn memory(argmem: readwrite, inaccessiblemem: readwrite) "frame-pointer"="none" }
attributes #8 = { nofree norecurse noreturn nosync nounwind memory(none) "frame-pointer"="none" }
attributes #9 = { nocallback nofree nounwind willreturn memory(argmem: write) "frame-pointer"="none" }
