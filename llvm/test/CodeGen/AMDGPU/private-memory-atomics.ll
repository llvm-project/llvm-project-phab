; RUN: llc -verify-machineinstrs -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tahiti < %s
; RUN: llc -verify-machineinstrs -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tonga < %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; This works because promote allocas pass replaces these with LDS atomics.

; Private atomics have no real use, but at least shouldn't crash on it.
define amdgpu_kernel void @atomicrmw_private(i32 addrspace(1)* %out, i32 %in) nounwind {
entry:
  %tmp = alloca [2 x i32], addrspace(5)
  %tmp1 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(5)* %tmp, i32 0, i32 0
  %tmp2 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(5)* %tmp, i32 0, i32 1
  store i32 0, i32 addrspace(5)* %tmp1
  store i32 1, i32 addrspace(5)* %tmp2
  %tmp3 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(5)* %tmp, i32 0, i32 %in
  %tmp4 = atomicrmw add i32 addrspace(5)* %tmp3, i32 7 acq_rel
  store i32 %tmp4, i32 addrspace(1)* %out
  ret void
}

define amdgpu_kernel void @cmpxchg_private(i32 addrspace(1)* %out, i32 %in) nounwind {
entry:
  %tmp = alloca [2 x i32], addrspace(5)
  %tmp1 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(5)* %tmp, i32 0, i32 0
  %tmp2 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(5)* %tmp, i32 0, i32 1
  store i32 0, i32 addrspace(5)* %tmp1
  store i32 1, i32 addrspace(5)* %tmp2
  %tmp3 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(5)* %tmp, i32 0, i32 %in
  %tmp4 = cmpxchg i32 addrspace(5)* %tmp3, i32 0, i32 1 acq_rel monotonic
  %val = extractvalue { i32, i1 } %tmp4, 0
  store i32 %val, i32 addrspace(1)* %out
  ret void
}
