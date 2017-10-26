; RUN: opt < %s -simplifycfg -mtriple=x86_64-unknown-linux-gnu -mcpu=corei7 -S | FileCheck %s
; Avoid if conversion if there is a long dependence chain.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"

define i64 @foo(i64** %pp, i64* %p) {
entry:
  %0 = load i64*, i64** %pp, align 8
  %1 = load i64, i64* %0, align 8
  %cmp = icmp slt i64 %1, 0
  %pint = ptrtoint i64* %p to i64
  br i1 %cmp, label %cond.true, label %cond.false

cond.true:
  %p1 = add i64 %pint, 8
  br label %cond.end

cond.false:
  %p2 = or i64 %pint, 16
  br label %cond.end

cond.end:
  %p3 = phi i64 [%p1, %cond.true], [%p2, %cond.false]
  %ptr = inttoptr i64 %p3 to i64*
  %val = load i64, i64* %ptr, align 8
  ret i64 %val

; CHECK-NOT: select
}

define i64 @bar(i64** %pp, i64* %p) {
entry:
  %0 = load i64*, i64** %pp, align 8
  %1 = load i64, i64* %0, align 8
  %cmp = icmp slt i64 %1, 0
  %pint = ptrtoint i64* %p to i64 
  br i1 %cmp, label %cond.true, label %cond.false

cond.true:
  %p1 = add i64 %pint, 8
  br label %cond.end

cond.false:
  %p2 = add i64 %pint, 16
  br label %cond.end

cond.end:
  %p3 = phi i64 [%p1, %cond.true], [%p2, %cond.false]
  %ptr = inttoptr i64 %p3 to i64*
  %val = load i64, i64* %ptr, align 8
  ret i64 %val

; CHECK-LABEL: @bar
; CHECK-NOT: select
}

