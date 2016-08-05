; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Check that the loop with indirect branches is rotated.
; CHECK-LABEL: foo
; CHECK: while.cond.lr:                                    ; preds = %entry
; CHECK: if.end6.lr:                                       ; preds = %while.cond.lr
; CHECK: if.else.lr:                                       ; preds = %if.end6.lr
; CHECK: if.end2.i.lr:                                     ; preds = %if.else.lr
; CHECK: if.end37.lr:                                      ; preds = %if.end2.i.lr
; CHECK: if.else96.lr:                                     ; preds = %if.end37.lr
; CHECK: if.else106.lr.ph:                                 ; preds = %if.else96.lr

define i32 @foo() {
entry:
  br label %while.cond

while.cond:                                       ; preds = %if.then467, %entry
  br i1 undef, label %if.then, label %if.end6

if.then:                                          ; preds = %while.cond
  unreachable

if.end6:                                          ; preds = %while.cond
  %conv7 = ashr exact i64 undef, 32
  br i1 undef, label %if.else, label %if.then19

if.then19:                                        ; preds = %if.end6
  unreachable

if.else:                                          ; preds = %if.end6
  br i1 undef, label %thr, label %if.end2.i

if.end2.i:                                        ; preds = %if.else
  br i1 undef, label %thr, label %if.end37

thr:                      ; preds = %if.end2.i, %if.else
  unreachable

if.end37:                                         ; preds = %if.end2.i
  br i1 undef, label %if.else96, label %if.then40

if.then40:                                        ; preds = %if.end37
  unreachable

if.else96:                                        ; preds = %if.end37
  br i1 undef, label %if.else106, label %if.then99

if.then99:                                        ; preds = %if.else96
  unreachable

if.else106:                                       ; preds = %if.else96
  switch i32 undef, label %if.else427 [
    i32 26, label %if.then130
    i32 24, label %if.then130
    i32 23, label %if.then130
    i32 22, label %if.then130
    i32 20, label %if.then130
    i32 19, label %if.then130
    i32 18, label %if.then130
    i32 12, label %if.then130
    i32 6, label %if.then149
    i32 30, label %if.then467
  ]

if.then130:                                       ; preds = %if.else106, %if.else106, %if.else106, %if.else106, %if.else106, %if.else106, %if.else106, %if.else106
  br i1 undef, label %land.lhs.true138, label %if.then467

land.lhs.true138:                                 ; preds = %if.then130
  unreachable

if.then149:                                       ; preds = %if.else106
  %add151 = add nsw i64 undef, %conv7
  unreachable

if.then467:                                       ; preds = %if.then130, %if.else106
  br label %while.cond
}
