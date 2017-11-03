; RUN: opt -S -mtriple=amdgcn-unknown-amdhsa-amdgiz -mcpu=kaveri -amdgpu-promote-alloca < %s | FileCheck %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; The types of the users of the addrspacecast should not be changed.

; CHECK-LABEL: @invalid_bitcast_addrspace(
; CHECK: getelementptr inbounds [256 x [1 x i32]], [256 x [1 x i32]] addrspace(3)* @invalid_bitcast_addrspace.data, i32 0, i32 %14
; CHECK: bitcast [1 x i32] addrspace(3)* %{{[0-9]+}} to half addrspace(3)*
; CHECK: addrspacecast half addrspace(3)* %tmp to half*
; CHECK: bitcast half* %tmp1 to <2 x i16>*
define amdgpu_kernel void @invalid_bitcast_addrspace() #0 {
entry:
  %data = alloca [1 x i32], align 4, addrspace(5)
  %tmp = bitcast [1 x i32] addrspace(5)* %data to half addrspace(5)*
  %tmp1 = addrspacecast half addrspace(5)* %tmp to half*
  %tmp2 = bitcast half* %tmp1 to <2 x i16>*
  %tmp3 = load <2 x i16>, <2 x i16>* %tmp2, align 2
  %tmp4 = bitcast <2 x i16> %tmp3 to <2 x half>
  ret void
}

attributes #0 = { nounwind }
