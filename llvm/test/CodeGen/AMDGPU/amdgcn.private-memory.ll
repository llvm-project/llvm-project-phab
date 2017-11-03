; RUN: llc -mattr=+promote-alloca -verify-machineinstrs -march=amdgcn -mtriple=amdgcn---amdgiz < %s | FileCheck -check-prefix=GCN -check-prefix=GCN-PROMOTE %s
; RUN: llc -mattr=+promote-alloca,-flat-for-global -verify-machineinstrs -mtriple=amdgcn--amdhsa-amdgiz -mcpu=kaveri < %s | FileCheck -check-prefix=GCN -check-prefix=GCN-PROMOTE -check-prefix=HSA %s
; RUN: llc -mattr=-promote-alloca -verify-machineinstrs -march=amdgcn -mtriple=amdgcn---amdgiz < %s | FileCheck -check-prefix=GCN -check-prefix=GCN-ALLOCA %s
; RUN: llc -mattr=-promote-alloca,-flat-for-global -verify-machineinstrs -mtriple=amdgcn-amdhsa-amdgiz -mcpu=kaveri < %s | FileCheck  -check-prefix=GCN -check-prefix=GCN-ALLOCA -check-prefix=HSA %s
; RUN: llc -mattr=+promote-alloca -verify-machineinstrs -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tonga -mattr=-flat-for-global < %s | FileCheck -check-prefix=GCN -check-prefix=GCN-PROMOTE %s
; RUN: llc -mattr=-promote-alloca -verify-machineinstrs -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tonga -mattr=-flat-for-global < %s | FileCheck -check-prefix=GCN  -check-prefix=GCN-ALLOCA %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"


declare i32 @llvm.amdgcn.workitem.id.x() nounwind readnone


; Make sure we don't overwrite workitem information with private memory

; GCN-LABEL: {{^}}work_item_info:
; GCN-NOT: v0
; GCN: v_add_i32_e32 [[RESULT:v[0-9]+]], vcc, v0, v{{[0-9]+}}
; GCN: buffer_store_dword [[RESULT]]
define amdgpu_kernel void @work_item_info(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0 = alloca [2 x i32], addrspace(5)
  %1 = getelementptr [2 x i32], [2 x i32] addrspace(5)* %0, i32 0, i32 0
  %2 = getelementptr [2 x i32], [2 x i32] addrspace(5)* %0, i32 0, i32 1
  store i32 0, i32 addrspace(5)* %1
  store i32 1, i32 addrspace(5)* %2
  %3 = getelementptr [2 x i32], [2 x i32] addrspace(5)* %0, i32 0, i32 %in
  %4 = load i32, i32 addrspace(5)* %3
  %5 = call i32 @llvm.amdgcn.workitem.id.x()
  %6 = add i32 %4, %5
  store i32 %6, i32 addrspace(1)* %out
  ret void
}
