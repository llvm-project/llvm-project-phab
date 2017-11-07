; RUN: llc < %s -march=amdgcn -mcpu=tonga -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=UNPACKED %s
; RUN: llc < %s -march=amdgcn -mcpu=gfx810 -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=PACKED %s
; RUN: llc < %s -march=amdgcn -mcpu=gfx901 -verify-machineinstrs | FileCheck -check-prefix=GCN -check-prefix=PACKED %s

; GCN-LABEL: {{^}}tbuffer_load_d16_x:
; GCN: tbuffer_load_format_d16_x v0, off, s[0:3],  dfmt:6,  nfmt:1, 0
define amdgpu_ps half @tbuffer_load_d16_x(<4 x i32> inreg %rsrc) {
main_body:
  %data = call half @llvm.amdgcn.tbuffer.load.f16(<4 x i32> %rsrc, i32 0, i32 0, i32 0, i32 0, i32 6, i32 1, i1 0, i1 0)
  ret half %data
}

; GCN-LABEL: {{^}}tbuffer_load_d16_xy:
; UNPACKED: tbuffer_load_format_d16_xy v[0:1], off, s[0:3],  dfmt:6,  nfmt:1, 0
; PACKED: tbuffer_load_format_d16_xy v0, off, s[0:3],  dfmt:6,  nfmt:1, 0
define amdgpu_ps <2 x half> @tbuffer_load_d16_xy(<4 x i32> inreg %rsrc) {
main_body:
  %data = call <2 x half> @llvm.amdgcn.tbuffer.load.v2f16(<4 x i32> %rsrc, i32 0, i32 0, i32 0, i32 0, i32 6, i32 1, i1 0, i1 0)
  ret < 2 x half> %data
}

; GCN-LABEL: {{^}}tbuffer_load_d16_xyzw:
; UNPACKED: tbuffer_load_format_d16_xyzw v[0:3], off, s[0:3],  dfmt:6,  nfmt:1, 0
; PACKED: tbuffer_load_format_d16_xyzw v[0:1], off, s[0:3],  dfmt:6,  nfmt:1, 0
define amdgpu_ps <4 x half> @tbuffer_load_d16_xyzw(<4 x i32> inreg %rsrc) {
main_body:
  %data = call <4 x half> @llvm.amdgcn.tbuffer.load.v4f16(<4 x i32> %rsrc, i32 0, i32 0, i32 0, i32 0, i32 6, i32 1, i1 0, i1 0)
  ret <4 x half> %data
}

declare half @llvm.amdgcn.tbuffer.load.f16(<4 x i32>, i32, i32, i32, i32, i32, i32, i1, i1)
declare <2 x half> @llvm.amdgcn.tbuffer.load.v2f16(<4 x i32>, i32, i32, i32, i32, i32, i32, i1, i1)
declare <4 x half> @llvm.amdgcn.tbuffer.load.v4f16(<4 x i32>, i32, i32, i32, i32, i32, i32, i1, i1)
