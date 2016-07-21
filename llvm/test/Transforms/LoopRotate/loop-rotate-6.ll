; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare i32 @foo(i8*, i32)

define i32 @bar(i8*, i32, i8*, i32) {
  switch i32 %3, label %6 [
    i32 0, label %25
    i32 -1, label %5
  ]

; <label>:5:                                      ; preds = %4
  unreachable

; <label>:6:                                      ; preds = %4
  br i1 undef, label %7, label %8

; <label>:7:                                      ; preds = %6
  br i1 undef, label %9, label %10

; <label>:8:                                      ; preds = %6
  unreachable

; <label>:9:                                      ; preds = %7
  unreachable

; <label>:10:                                     ; preds = %7
  br i1 undef, label %24, label %11

; <label>:11:                                     ; preds = %10
  switch i32 undef, label %12 [
    i32 46, label %21
    i32 32, label %21
  ]

; <label>:12:                                     ; preds = %11
  unreachable

; <label>:13:                                     ; preds = %21
  switch i8 undef, label %14 [
    i8 32, label %23
    i8 46, label %23
  ]

; <label>:14:                                     ; preds = %13
  br i1 undef, label %15, label %16

; <label>:15:                                     ; preds = %14
  unreachable

; <label>:16:                                     ; preds = %14
  br i1 undef, label %19, label %17

; <label>:17:                                     ; preds = %16
  %18 = call i32 @foo(i8* %22, i32 10) #4
  unreachable

; <label>:19:                                     ; preds = %16
  %20 = phi i8* [ %22, %16 ]
  br label %21

; <label>:21:                                     ; preds = %19, %11, %11
  %22 = phi i8* [ %20, %19 ], [ null, %11 ], [ null, %11 ]
  br i1 undef, label %23, label %13

; <label>:23:                                     ; preds = %21, %13, %13
  unreachable

; <label>:24:                                     ; preds = %10
  unreachable

; <label>:25:                                     ; preds = %4
  ret i32 0
}
