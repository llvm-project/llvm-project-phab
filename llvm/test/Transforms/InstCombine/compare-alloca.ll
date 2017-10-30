; RUN: opt -instcombine -S %s | FileCheck %s
target datalayout = "p:32:32"


define i1 @alloca_argument_compare(i64* %arg) {
  %alloc = alloca i64
  %cmp = icmp eq i64* %arg, %alloc
  ret i1 %cmp
  ; CHECK-LABEL: alloca_argument_compare
  ; CHECK: ret i1 false
}

define i1 @alloca_argument_compare_swapped(i64* %arg) {
  %alloc = alloca i64
  %cmp = icmp eq i64* %alloc, %arg
  ret i1 %cmp
  ; CHECK-LABEL: alloca_argument_compare_swapped
  ; CHECK: ret i1 false
}

define i1 @alloca_argument_compare_ne(i64* %arg) {
  %alloc = alloca i64
  %cmp = icmp ne i64* %arg, %alloc
  ret i1 %cmp
  ; CHECK-LABEL: alloca_argument_compare_ne
  ; CHECK: ret i1 true
}

define i1 @alloca_argument_compare_derived_ptrs(i64* %arg, i64 %x) {
  %alloc = alloca i64, i64 8
  %p = getelementptr i64, i64* %arg, i64 %x
  %q = getelementptr i64, i64* %alloc, i64 3
  %cmp = icmp eq i64* %p, %q
  ret i1 %cmp
  ; CHECK-LABEL: alloca_argument_compare_derived_ptrs
  ; CHECK: ret i1 false
}

declare void @escape(i64*)
define i1 @alloca_argument_compare_escaped_alloca(i64* %arg) {
  %alloc = alloca i64
  call void @escape(i64* %alloc)
  %cmp = icmp eq i64* %alloc, %arg
  ret i1 %cmp
  ; CHECK-LABEL: alloca_argument_compare_escaped_alloca
  ; CHECK: %cmp = icmp eq i64* %alloc, %arg
  ; CHECK: ret i1 %cmp
}

declare void @check_compares(i1, i1)
define void @alloca_argument_compare_two_compares(i64* %p) {
  %q = alloca i64, i64 8
  %r = getelementptr i64, i64* %p, i64 1
  %s = getelementptr i64, i64* %q, i64 2
  %cmp1 = icmp eq i64* %p, %q
  %cmp2 = icmp eq i64* %r, %s
  call void @check_compares(i1 %cmp1, i1 %cmp2)
  ret void
  ; We will only fold if there is a single cmp.
  ; CHECK-LABEL: alloca_argument_compare_two_compares
  ; CHECK: call void @check_compares(i1 %cmp1, i1 %cmp2)
}

define i1 @alloca_argument_compare_escaped_through_store(i64* %arg, i64** %ptr) {
  %alloc = alloca i64
  %cmp = icmp eq i64* %alloc, %arg
  %p = getelementptr i64, i64* %alloc, i64 1
  store i64* %p, i64** %ptr
  ret i1 %cmp
  ; CHECK-LABEL: alloca_argument_compare_escaped_through_store
  ; CHECK: %cmp = icmp eq i64* %alloc, %arg
  ; CHECK: ret i1 %cmp
}

declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture)
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture)
define i1 @alloca_argument_compare_benign_instrs(i8* %arg) {
  %alloc = alloca i8
  call void @llvm.lifetime.start.p0i8(i64 1, i8* %alloc)
  %cmp = icmp eq i8* %arg, %alloc
  %x = load i8, i8* %arg
  store i8 %x, i8* %alloc
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %alloc)
  ret i1 %cmp
  ; CHECK-LABEL: alloca_argument_compare_benign_instrs
  ; CHECK: ret i1 false
}

declare i64* @allocator()
define i1 @alloca_call_compare() {
  %p = alloca i64
  %q = call i64* @allocator()
  %cmp = icmp eq i64* %p, %q
  ret i1 %cmp
  ; CHECK-LABEL: alloca_call_compare
  ; CHECK: ret i1 false
}

; PR35102
define void @fold_alloca_cmp(i8* %visited) {
entry:
  %alc = alloca i8
  br label %loop

loop:
  %i = phi i32 [0, %entry], [%i_next, %loop_end]
  %p = inttoptr i32 %i to i8*
  %c = icmp eq i8* %alc, %p ; %c should not be folded to false.
  br i1 %c, label %matched, label %loop_end
matched:
  store i8 1, i8* %visited
  br label %loop_end
loop_end:
  %i_next = add i32 %i, 1
  %end = icmp eq i32 %i_next, -1
  br i1 %end, label %exit, label %loop
exit:
  ret void
; CHECK-LABEL: @fold_alloca_cmp(
; CHECK: store i8 1, i8* %visited
}
