; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define void @foo(i8*, i8*, i64) {
  %4 = and i32 undef, 7
  br label %5

; <label>:5:                                      ; preds = %10, %3
  %6 = phi i32 [ 0, %10 ], [ %4, %3 ]
  br i1 undef, label %11, label %7

; <label>:7:                                      ; preds = %5
  %8 = sub nsw i32 8, %6
  br i1 undef, label %9, label %10

; <label>:9:                                      ; preds = %7
  unreachable

; <label>:10:                                     ; preds = %7
  store i32 undef, i32* undef, align 8
  br label %5

; <label>:11:                                     ; preds = %5
  ret void
}
