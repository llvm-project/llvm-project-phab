; RUN: llc < %s -march=amdgcn -mcpu=tonga -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=UNPACKED %s
; RUN: llc < %s -march=amdgcn -mcpu=gfx810 -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=PACKED %s
; RUN: llc < %s -march=amdgcn -mcpu=gfx901 -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=PACKED %s


; GCN-LABEL: {{^}}tbuffer_store_d16_x:
; GCN: tbuffer_store_format_d16_x  v0, v1, s[4:7],  dfmt:1,  nfmt:2, 0 idxen
define amdgpu_kernel void @tbuffer_store_d16_x(<4 x i32> inreg %rsrc, half %data, i32 %vindex) {
main_body:
  call void @llvm.amdgcn.tbuffer.store.f16(half %data, <4 x i32> %rsrc, i32 %vindex, i32 0, i32 0, i32 0, i32 1, i32 2, i1 0, i1 0)
  ret void
}


; GCN-LABEL: {{^}}tbuffer_store_d16_xy:
; UNPACKED: tbuffer_store_format_d16_xy  v[1:2], v0, s[4:7],  dfmt:1,  nfmt:2, 0 idxen
; PACKED: tbuffer_store_format_d16_xy  v0, v1, s[4:7],  dfmt:1,  nfmt:2, 0 idxen
define amdgpu_kernel void @tbuffer_store_d16_xy(<4 x i32> inreg %rsrc, <2 x half> %data, i32 %vindex) {
main_body:
  call void @llvm.amdgcn.tbuffer.store.v2f16(<2 x half> %data, <4 x i32> %rsrc, i32 %vindex, i32 0, i32 0, i32 0, i32 1, i32 2, i1 0, i1 0)
  ret void
}


; GCN-LABEL: {{^}}tbuffer_store_d16_xyzw:
; UNPACKED: tbuffer_store_format_d16_xyzw  v[0:3], v4, s[4:7],  dfmt:1,  nfmt:2, 0 idxen
; PACKED: tbuffer_store_format_d16_xyzw  v[0:1], v2, s[4:7],  dfmt:1,  nfmt:2, 0 idxen
define amdgpu_kernel void @tbuffer_store_d16_xyzw(<4 x i32> inreg %rsrc, <4 x half> %data, i32 %vindex) {
main_body:
  call void @llvm.amdgcn.tbuffer.store.v4f16(<4 x half> %data, <4 x i32> %rsrc, i32 %vindex, i32 0, i32 0, i32 0, i32 1, i32 2, i1 0, i1 0)
  ret void
}

declare void @llvm.amdgcn.tbuffer.store.f16(half, <4 x i32>, i32, i32, i32, i32, i32, i32, i1, i1)
declare void @llvm.amdgcn.tbuffer.store.v2f16(<2 x half>, <4 x i32>, i32, i32, i32, i32, i32, i32, i1, i1)
declare void @llvm.amdgcn.tbuffer.store.v4f16(<4 x half>, <4 x i32>, i32, i32, i32, i32, i32, i32, i1, i1)