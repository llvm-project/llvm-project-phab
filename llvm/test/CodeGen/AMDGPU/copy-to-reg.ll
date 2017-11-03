; RUN: llc -march=amdgcn -mtriple=amdgcn---amdgiz -mattr=-promote-alloca -verify-machineinstrs < %s
; RUN: llc -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tonga -mattr=-flat-for-global -mattr=-promote-alloca -verify-machineinstrs < %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; Test that CopyToReg instructions don't have non-register operands prior
; to being emitted.

; Make sure this doesn't crash
; CHECK-LABEL: {{^}}copy_to_reg_frameindex:
define amdgpu_kernel void @copy_to_reg_frameindex(i32 addrspace(1)* %out, i32 %a, i32 %b, i32 %c) {
entry:
  %alloca = alloca [16 x i32], addrspace(5)
  br label %loop

loop:
  %inc = phi i32 [0, %entry], [%inc.i, %loop]
  %ptr = getelementptr [16 x i32], [16 x i32] addrspace(5)* %alloca, i32 0, i32 %inc
  store i32 %inc, i32 addrspace(5)* %ptr
  %inc.i = add i32 %inc, 1
  %cnd = icmp uge i32 %inc.i, 16
  br i1 %cnd, label %done, label %loop

done:
  %tmp0 = getelementptr [16 x i32], [16 x i32] addrspace(5)* %alloca, i32 0, i32 0
  %tmp1 = load i32, i32 addrspace(5)* %tmp0
  store i32 %tmp1, i32 addrspace(1)* %out
  ret void
}
