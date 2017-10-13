;RUN: llc < %s -march=amdgcn -mcpu=tonga -verify-machineinstrs | FileCheck -check-prefix=D16 %s
;RUN: llc < %s -march=amdgcn -mcpu=gfx901 -verify-machineinstrs | FileCheck -check-prefix=D16 %s


;D16-LABEL: {{^}}tbuffer_load_d16_x:
;D16: tbuffer_load_format_d16_x
;D16: s_waitcnt
define amdgpu_ps half @tbuffer_load_d16_x(<4 x i32> inreg %rsrc) {
main_body:
  %data = call half @llvm.amdgcn.tbuffer.load.f16(<4 x i32> %rsrc, i32 0, i32 0, i32 0, i32 0, i32 6, i32 1, i1 0, i1 0)
  ret half %data
}

;D16-LABEL: {{^}}tbuffer_load_d16_xy:
;D16: tbuffer_load_format_d16_xy
;D16: s_waitcnt
define amdgpu_ps <2 x half> @tbuffer_load_d16_xy(<4 x i32> inreg %rsrc) {
main_body:
  %data = call <2 x half> @llvm.amdgcn.tbuffer.load.v2f16(<4 x i32> %rsrc, i32 0, i32 0, i32 0, i32 0, i32 6, i32 1, i1 0, i1 0)
  ret < 2 x half> %data
}

;D16-LABEL: {{^}}tbuffer_load_d16_xyzw:
;D16: tbuffer_load_format_d16_xyzw
;D16: s_waitcnt
define amdgpu_ps <4 x half> @tbuffer_load_d16_xyzw(<4 x i32> inreg %rsrc) {
main_body:
  %data = call <4 x half> @llvm.amdgcn.tbuffer.load.v4f16(<4 x i32> %rsrc, i32 0, i32 0, i32 0, i32 0, i32 6, i32 1, i1 0, i1 0)
  ret <4 x half> %data
}

declare half @llvm.amdgcn.tbuffer.load.f16(<4 x i32>, i32, i32, i32, i32, i32, i32, i1, i1)
declare <2 x half> @llvm.amdgcn.tbuffer.load.v2f16(<4 x i32>, i32, i32, i32, i32, i32, i32, i1, i1)
declare <4 x half> @llvm.amdgcn.tbuffer.load.v4f16(<4 x i32>, i32, i32, i32, i32, i32, i32, i1, i1)
