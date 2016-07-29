; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Check that the loop is rotated.
; CHECK-LABEL: foo
; CHECK: bb3.lr:                                           ; preds = %bb
; CHECK: bb5.lr:                                           ; preds = %bb3.lr
; CHECK: bb8.lr.ph:                                        ; preds = %bb5.lr

define void @foo(i8* %arg, i8* %arg1, i64 %arg2) {
bb:
  %tmp = and i32 undef, 7
  br label %bb3

bb3:                                              ; preds = %bb8, %bb
  %tmp4 = phi i32 [ 0, %bb8 ], [ %tmp, %bb ]
  br i1 undef, label %bb9, label %bb5

bb5:                                              ; preds = %bb3
  %tmp6 = sub nsw i32 8, %tmp4
  br i1 undef, label %bb7, label %bb8

bb7:                                              ; preds = %bb5
  unreachable

bb8:                                              ; preds = %bb5
  store i32 undef, i32* undef, align 8
  br label %bb3

bb9:                                              ; preds = %bb3
  ret void
}
