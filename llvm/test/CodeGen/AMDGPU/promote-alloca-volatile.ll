; RUN: opt -S -mtriple=amdgcn-unknown-amdhsa-amdgiz -amdgpu-promote-alloca < %s | FileCheck %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; CHECK-LABEL: @volatile_load(
; CHECK: alloca [4 x i32]
; CHECK: load volatile i32, i32 addrspace(5)*
define amdgpu_kernel void @volatile_load(i32 addrspace(1)* nocapture %out, i32 addrspace(1)* nocapture %in) {
entry:
  %stack = alloca [4 x i32], align 4, addrspace(5)
  %tmp = load i32, i32 addrspace(1)* %in, align 4
  %arrayidx1 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(5)* %stack, i32 0, i32 %tmp
  %load = load volatile i32, i32 addrspace(5)* %arrayidx1
  store i32 %load, i32 addrspace(1)* %out
 ret void
}

; CHECK-LABEL: @volatile_store(
; CHECK: alloca [4 x i32]
; CHECK: store volatile i32 %tmp, i32 addrspace(5)*
define amdgpu_kernel void @volatile_store(i32 addrspace(1)* nocapture %out, i32 addrspace(1)* nocapture %in) {
entry:
  %stack = alloca [4 x i32], align 4, addrspace(5)
  %tmp = load i32, i32 addrspace(1)* %in, align 4
  %arrayidx1 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(5)* %stack, i32 0, i32 %tmp
  store volatile i32 %tmp, i32 addrspace(5)* %arrayidx1
 ret void
}

; Has on OK non-volatile user but also a volatile user
; CHECK-LABEL: @volatile_and_non_volatile_load(
; CHECK: alloca double
; CHECK: load double
; CHECK: load volatile double
define amdgpu_kernel void @volatile_and_non_volatile_load(double addrspace(1)* nocapture %arg, i32 %arg1) #0 {
bb:
  %tmp = alloca double, align 8, addrspace(5)
  store double 0.000000e+00, double addrspace(5)* %tmp, align 8

  %tmp4 = load double, double addrspace(5)* %tmp, align 8
  %tmp5 = load volatile double, double addrspace(5)* %tmp, align 8

  store double %tmp4, double addrspace(1)* %arg
  ret void
}

attributes #0 = { nounwind }
