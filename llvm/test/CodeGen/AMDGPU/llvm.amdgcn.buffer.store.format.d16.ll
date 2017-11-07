; RUN: llc < %s -march=amdgcn -mcpu=tonga -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=UNPACKED %s
; RUN: llc < %s -march=amdgcn -mcpu=gfx810 -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=PACKED %s
; RUN: llc < %s -march=amdgcn -mcpu=gfx901 -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=PACKED %s

; GCN-LABEL: {{^}}buffer_store_format_d16_x:
; GCN: buffer_store_format_d16_x v0, v1, s[4:7], 0 idxen
define amdgpu_kernel void @buffer_store_format_d16_x(<4 x i32> inreg %rsrc, half %data, i32 %index) {
main_body:
  call void @llvm.amdgcn.buffer.store.format.f16(half %data, <4 x i32> %rsrc, i32 %index, i32 0, i1 0, i1 0)
  ret void
}

; GCN-LABEL: {{^}}buffer_store_format_d16_xy:
; UNPACKED: buffer_store_format_d16_xy v[1:2], v0, s[4:7], 0 idxen
; PACKED: buffer_store_format_d16_xy v0, v1, s[4:7], 0 idxen
define amdgpu_kernel void @buffer_store_format_d16_xy(<4 x i32> inreg %rsrc, <2 x half> %data, i32 %index) {
main_body:
  call void @llvm.amdgcn.buffer.store.format.v2f16(<2 x half> %data, <4 x i32> %rsrc, i32 %index, i32 0, i1 0, i1 0)
  ret void
}

; GCN-LABEL: {{^}}buffer_store_format_d16_xyzw:
; UNPACKED: buffer_store_format_d16_xyzw v[0:3], v4, s[4:7], 0 idxen
; PACKED: buffer_store_format_d16_xyzw v[0:1], v2, s[4:7], 0 idxen
define amdgpu_kernel void @buffer_store_format_d16_xyzw(<4 x i32> inreg %rsrc, <4 x half> %data, i32 %index) {
main_body:
  call void @llvm.amdgcn.buffer.store.format.v4f16(<4 x half> %data, <4 x i32> %rsrc, i32 %index, i32 0, i1 0, i1 0)
  ret void
}

declare void @llvm.amdgcn.buffer.store.format.f16(half, <4 x i32>, i32, i32, i1, i1)
declare void @llvm.amdgcn.buffer.store.format.v2f16(<2 x half>, <4 x i32>, i32, i32, i1, i1)
declare void @llvm.amdgcn.buffer.store.format.v4f16(<4 x half>, <4 x i32>, i32, i32, i1, i1)
