; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Check that the loop is rotated.
; CHECK: do.body.i10.lr:                                   ; preds = %if.else.i3
; CHECK: do.cond.i19.lr.ph:                                ; preds = %do.body.i10.lr

define void @foo() {
entry:
  br i1 undef, label %for.body, label %for.end

for.body:                                         ; preds = %entry
  br i1 undef, label %inverse.exit21, label %if.else.i3

if.else.i3:                                       ; preds = %for.body
  br label %do.body.i10

do.body.i10:                                      ; preds = %do.cond.i19, %if.else.i3
  %b1.0.i4 = phi i64 [ 0, %if.else.i3 ], [ %b2.0.i7, %do.cond.i19 ]
  %b2.0.i7 = phi i64 [ 1, %if.else.i3 ], [ %sub8.i18, %do.cond.i19 ]
  br i1 undef, label %do.cond.thread.i15, label %do.cond.i19

do.cond.thread.i15:                               ; preds = %do.body.i10
  br label %inverse.exit21

do.cond.i19:                                      ; preds = %do.body.i10
  %mul.i17 = mul nsw i64 undef, %b2.0.i7
  %sub8.i18 = sub nsw i64 %b1.0.i4, %mul.i17
  br label %do.body.i10

inverse.exit21:                                   ; preds = %do.cond.thread.i15, %for.body
  br label %for.end

for.end:                                          ; preds = %inverse.exit21, %entry
  ret void
}
