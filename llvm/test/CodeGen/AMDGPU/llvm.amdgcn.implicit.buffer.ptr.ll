; RUN: llc -mtriple=amdgcn-mesa-mesa3d-amdgiz -verify-machineinstrs < %s | FileCheck -check-prefix=GCN %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; FIXME: Requires stack object to not assert
; GCN-LABEL: {{^}}test_ps:
; GCN: s_load_dwordx2 s[4:5], s[0:1], 0x0
; GCN: buffer_store_dword v0, off, s[4:7], s2 offset:4
; GCN: s_load_dword s{{[0-9]+}}, s[0:1], 0x0
; GCN-NEXT: s_waitcnt
; GCN-NEXT: ; return
define amdgpu_ps i32 @test_ps() #1 {
  %alloca = alloca i32, addrspace(5)
  store volatile i32 0, i32 addrspace(5)* %alloca
  %implicit_buffer_ptr = call i8 addrspace(2)* @llvm.amdgcn.implicit.buffer.ptr()
  %buffer_ptr = bitcast i8 addrspace(2)* %implicit_buffer_ptr to i32 addrspace(2)*
  %value = load volatile i32, i32 addrspace(2)* %buffer_ptr
  ret i32 %value
}

; GCN-LABEL: {{^}}test_cs:
; GCN: s_mov_b64 s[4:5], s[0:1]
; GCN: buffer_store_dword v{{[0-9]+}}, off, s[4:7], s2 offset:4
; GCN: s_load_dword s0, s[0:1], 0x0
define amdgpu_cs i32 @test_cs() #1 {
  %alloca = alloca i32, addrspace(5)
  store volatile i32 0, i32 addrspace(5)* %alloca
  %implicit_buffer_ptr = call i8 addrspace(2)* @llvm.amdgcn.implicit.buffer.ptr()
  %buffer_ptr = bitcast i8 addrspace(2)* %implicit_buffer_ptr to i32 addrspace(2)*
  %value = load volatile i32, i32 addrspace(2)* %buffer_ptr
  ret i32 %value
}

declare i8 addrspace(2)* @llvm.amdgcn.implicit.buffer.ptr() #0

attributes #0 = { nounwind readnone speculatable }
attributes #1 = { nounwind }
