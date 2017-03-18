; RUN: opt -S -indvars < %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @f_0(i32* %ptr) {
; CHECK-LABEL: @f_0(
; CHECK: for.cond:
; CHECK:    %iv = phi i32 [ -2, %entry ], [ %iv.inc, %for.cond ]
; CHECK:    %iv.inc = add i32 %iv, 1
; CHECK:    %exitcond = icmp ne i32 %iv.inc, 0
; CHECK:    br i1 %exitcond, label %for.cond, label %for.end
;
entry:
  br label %for.cond

for.cond:
  %iv = phi i32 [ -2, %entry ], [ %iv.inc, %for.cond ]
  store i32 %iv, i32* %ptr
  %cmp = icmp slt i32 %iv, -1
  %iv.inc = add nuw nsw i32 %iv, 1
  br i1 %cmp, label %for.cond, label %for.end

for.end:
  ret i32 0
}

define i32 @f_1(i32* %ptr, i1 %always_false) {
; CHECK-LABEL: @f_1(
; CHECK: always_taken:
; CHECK:   %iv.inc = add nsw i32 %iv, 1
; CHECK:   %iv2.inc = add i32 %iv2, 1
; CHECK:   %exitcond = icmp ne i32 %iv2.inc, -2147483627
; CHECK:   br i1 %exitcond, label %for.cond, label %for.end

entry:
  br label %for.cond

for.cond:
  %iv = phi i32 [ -2147483648, %entry ], [ %iv.inc, %always_taken ]
  %iv2 = phi i32 [ 0, %entry ], [ %iv2.inc, %always_taken ]
  store i32 %iv, i32* %ptr
  br i1 %always_false, label %never_taken, label %always_taken

never_taken:
  store volatile i32 %iv2, i32* %ptr
  br label %always_taken

always_taken:
  %iv.inc = add nsw i32 %iv, 1
  %iv2.inc = add nsw i32 %iv2, 1
  %cmp = icmp slt i32 %iv, 20
  br i1 %cmp, label %for.cond, label %for.end

for.end:
  ret i32 0
}

define i32 @f_2(i32* %ptr, i1 %always_false) {
; CHECK-LABEL: @f_2(
; CHECK: always_taken:
; CHECK:  %iv.inc = add nuw nsw i32 %iv, 1
; CHECK:  %exitcond = icmp ne i32 %iv.inc, 25
; CHECK:  br i1 %exitcond, label %for.cond, label %for.end

entry:
  br label %for.cond

for.cond:
  %iv = phi i32 [ 0, %entry ], [ %iv.inc, %always_taken ]
  store i32 %iv, i32* %ptr
  br i1 %always_false, label %never_taken, label %always_taken

never_taken:
  store volatile i32 %iv, i32* %ptr
  br label %always_taken

always_taken:
  %iv.inc = add nuw nsw i32 %iv, 1
  %iv.offset = add i32 %iv.inc, -5
  %cmp = icmp slt i32 %iv.offset, 20
  br i1 %cmp, label %for.cond, label %for.end

for.end:
  ret i32 0
}

define i32 @f_3(i32* %ptr, i1 %always_false) {
; CHECK-LABEL: @f_3(
; CHECK: always_taken:
; CHECK:   %iv.inc = add i32 %iv, 1
; CHECK:   %exitcond = icmp ne i32 %iv.inc, 21
; CHECK:   br i1 %exitcond, label %for.cond, label %for.end

entry:
  br label %for.cond

for.cond:
  %iv = phi i32 [ 0, %entry ], [ %iv.inc, %always_taken ]
  store i32 %iv, i32* %ptr
  br i1 %always_false, label %never_taken, label %always_taken

never_taken:
  store volatile i32 %iv, i32* %ptr
  br label %always_taken

always_taken:
  %iv.inc = add nuw nsw i32 %iv, 1
  %cmp = icmp slt i32 %iv, 20
  br i1 %cmp, label %for.cond, label %for.end

for.end:
  ret i32 0
}
