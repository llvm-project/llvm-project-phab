; RUN: llc -O0 -march=amdgcn -mtriple=amdgcn---amdgiz -verify-machineinstrs < %s | FileCheck %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; Test that the alignment of kernel arguments does not impact the
; alignment of the stack

; CHECK-LABEL: {{^}}no_args:
; CHECK: ScratchSize: 5{{$}}
define amdgpu_kernel void @no_args() {
  %alloca = alloca i8, addrspace(5)
  store volatile i8 0, i8 addrspace(5)* %alloca
  ret void
}

; CHECK-LABEL: {{^}}force_align32:
; CHECK: ScratchSize: 5{{$}}
define amdgpu_kernel void @force_align32(<8 x i32>) {
  %alloca = alloca i8, addrspace(5)
  store volatile i8 0, i8 addrspace(5)* %alloca
  ret void
}

; CHECK-LABEL: {{^}}force_align64:
; CHECK: ScratchSize: 5{{$}}
define amdgpu_kernel void @force_align64(<16 x i32>) {
  %alloca = alloca i8, addrspace(5)
  store volatile i8 0, i8 addrspace(5)* %alloca
  ret void
}

; CHECK-LABEL: {{^}}force_align128:
; CHECK: ScratchSize: 5{{$}}
define amdgpu_kernel void @force_align128(<32 x i32>) {
  %alloca = alloca i8, addrspace(5)
  store volatile i8 0, i8 addrspace(5)* %alloca
  ret void
}

; CHECK-LABEL: {{^}}force_align256:
; CHECK: ScratchSize: 5{{$}}
define amdgpu_kernel void @force_align256(<64 x i32>) {
  %alloca = alloca i8, addrspace(5)
  store volatile i8 0, i8 addrspace(5)* %alloca
  ret void
}
