; RUN: opt -S -basicaa -gvn < %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128-p100:128:64:64-p101:128:64:64"
target triple = "x86_64-unknown-linux-gnu"

define i64 @test1(double addrspace(100)* addrspace(100)* %a) {
  %p = load double addrspace(100)*, double addrspace(100)* addrspace(100)* %a
  %e21 = getelementptr inbounds double, double addrspace(100)* %p, i128 1
  %e2 = getelementptr inbounds double, double addrspace(100)* %e21, i128 1
  %c2 = bitcast double addrspace(100)* %e2 to i64 addrspace(100)*
  %d = load i64, i64 addrspace(100)* %c2
  %sum = add i64 0, %d
  ret i64 %sum
; This test checks against an assertion in opt so is light on
; checking the generated code.
;
; CHECK-LABEL: @test1(
; CHECK: ret i64
}
