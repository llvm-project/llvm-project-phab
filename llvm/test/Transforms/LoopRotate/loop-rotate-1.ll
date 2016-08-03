; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Check that the loop is rotated.
; CHECK: while.cond.lr:                                    ; preds = %entry
; CHECK: while.body.lr.ph:                                 ; preds = %while.cond.lr

%fp = type { i64 }

declare void @foo(%fp* %this, %fp* %that)

define void @bar(%fp* %__begin1, %fp* %__end1, %fp** %__end2) {
entry:
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %__end1.addr.0 = phi %fp* [ %__end1, %entry ], [ %incdec.ptr, %while.body ]
  %cmp = icmp eq %fp* %__end1.addr.0, %__begin1
  br i1 %cmp, label %while.end, label %while.body

while.body:                                       ; preds = %while.cond
  %0 = load %fp*, %fp** %__end2, align 8
  %add.ptr = getelementptr inbounds %fp, %fp* %0, i64 -1
  %incdec.ptr = getelementptr inbounds %fp, %fp* %__end1.addr.0, i64 -1
  tail call void @foo(%fp* %add.ptr, %fp* %incdec.ptr)
  %1 = load %fp*, %fp** %__end2, align 8
  %incdec.ptr2 = getelementptr inbounds %fp, %fp* %1, i64 -1
  store %fp* %incdec.ptr2, %fp** %__end2, align 8
  br label %while.cond

while.end:                                        ; preds = %while.cond
  ret void
}
