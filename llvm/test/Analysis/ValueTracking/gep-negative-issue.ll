; RUN: opt -gvn -S < %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128-p100:128:64:64-p101:128:64:64"
target triple = "x86_64-unknown-linux-gnu"

%_array = type { i64, %ArrayImpl addrspace(100)*, i8 }

%ArrayImpl = type { i64, i64 addrspace(100)*, [1 x i64], [1 x i64], [1 x i64], i64, i64, double addrspace(100)*, double addrspace(100)*, i8, i64 }

define void @test(i64 %n_chpl) {
entry:
  %0 = getelementptr inbounds %_array, %_array* null, i32 0, i32 1
  %1 = load %ArrayImpl addrspace(100)*, %ArrayImpl addrspace(100)** %0
  %2 = getelementptr inbounds %ArrayImpl, %ArrayImpl addrspace(100)* %1, i32 0, i32 8
  %3 = load double addrspace(100)*, double addrspace(100)* addrspace(100)* %2
  %4 = getelementptr inbounds double, double addrspace(100)* %3, i64 -1
  store double 0.000000e+00, double addrspace(100)* %4
  %5 = load %ArrayImpl addrspace(100)*, %ArrayImpl addrspace(100)** %0
  %6 = getelementptr inbounds %ArrayImpl, %ArrayImpl addrspace(100)* %5, i32 0, i32 8
  %7 = load double addrspace(100)*, double addrspace(100)* addrspace(100)* %6
  %8 = getelementptr inbounds double, double addrspace(100)* %7, i64 %n_chpl
  store double 0.000000e+00, double addrspace(100)* %8
  ret void
; This test checks against an assertion in opt so is light on
; checking the generated code.
;
; CHECK-LABEL: @test(
; CHECK: ret void
}
