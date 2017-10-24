; RUN: opt -verify-loop-info -irce-print-changed-loops -irce -S < %s 2>&1 | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128-ni:1"
target triple = "x86_64-unknown-linux-gnu"

; IRCE should fail here because the preheader's exiting value is a phi from the
; loop, and this value cannot be expanded at loop's preheader.

; Function Attrs: uwtable
define void @test_01() {

; CHECK-NOT:   irce: in function test_01: constrained Loop

; CHECK-LABEL: test_01
; CHECK-NOT:   preloop
; CHECK-NOT:   postloop
; CHECK-NOT:   br i1 false
; CHECK-NOT:   br i1 true

entry:
  br label %loop

exit:                                       ; preds = %guarded, %loop
  ret void

loop:                                      ; preds = %guarded, %entry
  %indvars.iv = phi i64 [ 380, %entry ], [ %indvars.iv.next, %guarded ]
  %tmp1 = phi i32 [ 3, %entry ], [ %tmp2, %guarded ]
  %tmp2 = add nuw nsw i32 %tmp1, 1
  %tmp3 = add nuw nsw i64 %indvars.iv, 1
  %tmp4 = icmp slt i64 %tmp3, 5
  br i1 %tmp4, label %guarded, label %exit

guarded:                                          ; preds = %tmp0
  %indvars.iv.next = add nsw i64 %indvars.iv, -1
  %tmp5 = add nuw nsw i32 %tmp1, 8
  %tmp6 = zext i32 %tmp5 to i64
  %tmp7 = icmp eq i64 %indvars.iv.next, %tmp6
  br i1 %tmp7, label %exit, label %loop
}
