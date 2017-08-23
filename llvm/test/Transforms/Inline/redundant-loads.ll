; REQUIRES: asserts
; RUN: opt -inline < %s -S -debug-only=inline-cost 2>&1  | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK      Analyzing call of inner1... (caller:outer1)
; CHECK      LoadEliminationCost: 5
define void @outer1(i32* %a) {
  %b = alloca i32
  call void @inner1(i32* %a, i32* %b)
  ret void
}

define void @inner1(i32* %a, i32* %b) {
  %1 = load i32, i32* %a
  store i32 %1, i32 * %b
  %2 = load i32, i32* %a
  ret void
}

; CHECK:      Analyzing call of inner2... (caller:outer2)
; CHECK:      LoadEliminationCost: 0
define void @outer2(i32* %a, i32* %b) {
  call void @inner2(i32* %a, i32* %b)
  ret void
}

define void @inner2(i32* %a, i32* %b) {
  %1 = load i32, i32* %a
  store i32 %1, i32 * %b
  %2 = load i32, i32* %a
  ret void
}

; CHECK      Analyzing call of inner3... (caller:outer3)
; CHECK      LoadEliminationCost: 0
define void @outer3(i32* %a) {
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

; CHECK      Analyzing call of inner4... (caller:outer4)
; CHECK      LoadEliminationCost: 5
define void @outer4(i32* %a, i32* %b, i32* %c) {
  call void @inner4(i32* %a, i32* %b, i1 false)
  ret void
}

define void @inner4(i32* %a, i32* %b, i1 %pred) {
  %1 = load i32, i32* %a
  br i1 %pred, label %cond_true, label %cond_false

cond_true:
  store i32 %1, i32 * %b
  br label %cond_false

cond_false:
  %2 = load i32, i32* %a
  ret void
}
