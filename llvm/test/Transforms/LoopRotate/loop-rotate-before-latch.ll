; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck %s
; RUN: opt -S < %s  -loop-rotate -verify-dom-info -verify-loop-info -loop-rotate | FileCheck %s -check-prefix=CHECK1

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK-LABEL:foo
; CHECK: "llvm.loop.rotated.times", i32 1

; Check that the loop is rotated only once despite multiple invocations of loop-rotate on a loop.
; CHECK1-LABEL:foo
; CHECK1: "llvm.loop.rotated.times", i32 1

; Function Attrs: nounwind uwtable
define hidden void @foo(i32 %arg1, i32 %arg3) local_unnamed_addr {

entry:
  br label %bb25

bb25:                                             ; preds = %bb140, %entry
  br i1 undef, label %bb165, label %bb34

bb34:                                             ; preds = %bb25
  switch i32 undef, label %bb35 [
    i32 46, label %bb36
    i32 32, label %bb36
  ]

bb35:                                             ; preds = %bb34
  unreachable

bb36:                                             ; preds = %bb34, %bb34
  br label %bb140

bb140:                                            ; preds = %bb36
  %tmp145 = add nsw i32 undef, undef
  br i1 undef, label %bb25, label %bb146

bb146:                                            ; preds = %bb140
  %tmp147 = icmp sgt i32 %tmp145, %arg1
  unreachable

bb165:                                            ; preds = %bb25
  unreachable
}

