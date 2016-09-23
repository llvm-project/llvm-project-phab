; RUN: opt -S -loop-rotate < %s -verify-loop-info -verify-dom-info | FileCheck %s
; ModuleID = 'bugpoint-reduced-simplified.bc'
source_filename = "bugpoint-output-fdebd3b.bc"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.8.0"

; Function Attrs: nounwind readonly
define void @test1() #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.cond1, %entry
  %sum.0 = phi i32 [ 0, %entry ], [ %sum.1, %for.cond1 ]
  br i1 undef, label %for.cond1, label %return

for.cond1:                                        ; preds = %land.rhs, %for.cond
  %sum.1 = phi i32 [ 0, %land.rhs ], [ %sum.0, %for.cond ]
  br i1 undef, label %land.rhs, label %for.cond

land.rhs:                                         ; preds = %for.cond1
  br i1 undef, label %return, label %for.cond1

return:                                           ; preds = %land.rhs, %for.cond
  %retval.0 = phi i32 [ 1000, %land.rhs ], [ %sum.0, %for.cond ]
  ret void
}

attributes #0 = { nounwind readonly }

; check that loop with multiple exit is rotated and has a preheader.
; CHECK-LABEL: test1
; CHECK: entry:
; CHECK: br label %for.cond.lr
; CHECK: for.cond.loopexit:
; CHECK:   br label %for.cond
; CHECK: for.cond.lr:
; CHECK:   br i1 false, label %for.cond1.preheader.lr.ph, label %return.loopexit1
; CHECK: for.cond1.preheader.lr.ph:
; CHECK:   br label %for.cond1.preheader
; CHECK: for.cond1.preheader:
