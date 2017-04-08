; RUN: opt %loadPolly -polly-codegen -S -polly-invariant-load-hoisting=true \
; RUN: < %s | FileCheck %s
;
; Check load-hoisting from pointers that by themselves are defined within the
; SCoP, but are themselves invariant.
;
; CHECK: store i32 %polly.access.polly.access.polly.access.A.load.load.load, i32* %B
;
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-unknown"

define void @f(i32*** %A, i32* %B) {
entry:
  br label %bb1

bb1:                                              ; preds = %bb1, %entry
  %i = phi i64 [ %i.next, %bb1 ], [ 0, %entry ]
  %tmp1 = load i32**, i32*** %A, align 8
  %tmp2 = load i32*, i32** %tmp1, align 8
  %tmp3 = load i32, i32* %tmp2, align 8
  store i32 %tmp3, i32* %B, align 8
  %i.next = add i64 %i, 1
  %exitcond = icmp eq i64 %i.next, 4
  br i1 %exitcond, label %return, label %bb1

return:                                             ; preds = %bb1
  ret void
}
