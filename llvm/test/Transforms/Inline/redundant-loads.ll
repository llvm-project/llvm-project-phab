; RUN: opt -inline < %s -S -o - -inline-threshold=0 | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define void @outer1(i32* %a) {
; CHECK-LABEL: @outer1(
; CHECK-NOT: call void @inner1
  %b = alloca i32
  call void @inner1(i32* %a, i32* %b)
  ret void
}

define void @inner1(i32* %a, i32* %b) {
  %1 = load i32, i32* %a
  store i32 %1, i32 * %b 
  %2 = load i32, i32* %a
  store i32 %2, i32 * %b 
  %3 = load i32, i32* %a
  store i32 %3, i32 * %b 
  %4 = load i32, i32* %a
  store i32 %4, i32 * %b
  %5 = load i32, i32* %a
  store i32 %5, i32 * %b
  %6 = load i32, i32* %a
  store i32 %6, i32 * %b
  %7 = load i32, i32* %a
  store i32 %7, i32 * %b
  %8 = load i32, i32* %a
  store i32 %8, i32 * %b
  %9 = load i32, i32* %a
  ret void
}

define void @outer2(i32* %a, i32* %b) {
; CHECK-LABEL: @outer2(
; CHECK: call void @inner2
  call void @inner2(i32* %a, i32* %b)
  ret void
}

define void @inner2(i32* %a, i32* %b) {
  %1 = load i32, i32* %a
  store i32 %1, i32 * %b
  %2 = load i32, i32* %a
  store i32 %2, i32 * %b
  %3 = load i32, i32* %a
  store i32 %3, i32 * %b
  %4 = load i32, i32* %a
  store i32 %4, i32 * %b
  %5 = load i32, i32* %a
  ret void
}

define void @outer3(i32* %a) {
; CHECK-LABEL: @outer3(
; CHECK: call void @inner3
  call void @inner3(i32* %a)
  ret void
}

declare void @foo()

define void @inner3(i32* %a) {
  %1 = load i32, i32* %a
  call void @foo()
  %2 = load i32, i32* %a
  ret void
}

define void @outer4(i32* %a, i32* %b, i32* %c) {
; CHECK-LABEL: @outer4(
; CHECK-NOT: call void @inner4
  call void @inner4(i32* %a, i32* %b, i32* %c, i1 false)
  ret void
}

define void @inner4(i32* %a, i32* %b, i32* %c, i1 %pred) {
  %1 = load i32, i32* %a
  %2 = load i32, i32* %b
  %3 = load i32, i32* %c
  br i1 %pred, label %cond_true, label %cond_false

cond_true: 
  store i32 %1, i32 * %b 
  br label %cond_false

cond_false:
  %4 = load i32, i32* %a
  %5 = load i32, i32* %b
  %6 = load i32, i32* %c
  %7 = add i32 %1, %2
  %8 = add i32 %3, %7
  %9 = add i32 %4, %8
  %10 = add i32 %5, %9
  %11 = add i32 %6, %10
  ret void
}
