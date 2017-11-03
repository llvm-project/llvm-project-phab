; RUN: llc -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tahiti -mattr=-promote-alloca -verify-machineinstrs < %s
; RUN: llc -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tonga -mattr=-promote-alloca -verify-machineinstrs < %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; Test that INSERT_SUBREG instructions don't have non-register operands after
; instruction selection.

; Make sure this doesn't crash
; CHECK-LABEL: test:
define amdgpu_kernel void @test(i64 addrspace(1)* %out) {
entry:
  %tmp0 = alloca [16 x i32], addrspace(5)
  %tmp1 = ptrtoint [16 x i32] addrspace(5)* %tmp0 to i32
  %tmp2 = sext i32 %tmp1 to i64
  store i64 %tmp2, i64 addrspace(1)* %out
  ret void
}
