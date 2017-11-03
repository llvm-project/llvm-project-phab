;RUN: llc < %s -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=verde -mattr=+vgpr-spilling -mattr=-promote-alloca -verify-machineinstrs | FileCheck %s -check-prefix=CHECK
;RUN: llc < %s -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tonga -mattr=-flat-for-global -mattr=+vgpr-spilling -mattr=-promote-alloca -verify-machineinstrs | FileCheck %s -check-prefix=CHECK
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; Allocate two stack slots of 2052 bytes each requiring a total of 4104 bytes.
; Extracting the last element of each does not fit into the offset field of
; MUBUF instructions, so a new base register is needed. This used to not
; happen, leading to an assertion.

; CHECK-LABEL: {{^}}main:
; CHECK: buffer_store_dword
; CHECK: buffer_store_dword
; CHECK: buffer_load_dword
; CHECK: buffer_load_dword
define amdgpu_gs float @main(float %v1, float %v2, i32 %idx1, i32 %idx2) {
main_body:
  %m1 = alloca [513 x float], addrspace(5)
  %m2 = alloca [513 x float], addrspace(5)

  %gep1.store = getelementptr [513 x float], [513 x float] addrspace(5)* %m1, i32 0, i32 %idx1
  store float %v1, float addrspace(5)* %gep1.store

  %gep2.store = getelementptr [513 x float], [513 x float] addrspace(5)* %m2, i32 0, i32 %idx2
  store float %v2, float addrspace(5)* %gep2.store

; This used to use a base reg equal to 0.
  %gep1.load = getelementptr [513 x float], [513 x float] addrspace(5)* %m1, i32 0, i32 0
  %out1 = load float, float addrspace(5)* %gep1.load

; This used to attempt to re-use the base reg at 0, generating an out-of-bounds instruction offset.
  %gep2.load = getelementptr [513 x float], [513 x float] addrspace(5)* %m2, i32 0, i32 512
  %out2 = load float, float addrspace(5)* %gep2.load

  %r = fadd float %out1, %out2
  ret float %r
}
