; RUN: not llc -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=bonaire -mattr=-promote-alloca < %s 2>&1 | FileCheck -check-prefix=ERROR %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; ERROR: error: <unknown>:0:0: in function use_group_to_global_addrspacecast void (i32 addrspace(3)*): invalid addrspacecast
define amdgpu_kernel void @use_group_to_global_addrspacecast(i32 addrspace(3)* %ptr) {
  %stof = addrspacecast i32 addrspace(3)* %ptr to i32 addrspace(1)*
  store volatile i32 0, i32 addrspace(1)* %stof
  ret void
}
