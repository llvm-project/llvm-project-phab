; RUN: llc -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tonga -mattr=-flat-for-global -verify-machineinstrs -show-mc-encoding < %s | FileCheck -check-prefix=VI -check-prefix=VI-OPT %s
; RUN: llc -O0 -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tonga -mattr=-flat-for-global -verify-machineinstrs -show-mc-encoding < %s | FileCheck -check-prefix=VI -check-prefix=VI-NOOPT %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; FIXME: The register allocator / scheduler should be able to avoid these hazards.

; VI-LABEL: {{^}}dpp_test:
; VI: v_mov_b32_e32 v0, s{{[0-9]+}}
; VI-NOOPT: v_mov_b32_e32 v1, s{{[0-9]+}}
; VI: s_nop 1
; VI-OPT: v_mov_b32_dpp v0, v0 quad_perm:[1,0,0,0] row_mask:0x1 bank_mask:0x1 bound_ctrl:0 ; encoding: [0xfa,0x02,0x00,0x7e,0x00,0x01,0x08,0x11]
; VI-NOOPT: v_mov_b32_dpp v0, v1 quad_perm:[1,0,0,0] row_mask:0x1 bank_mask:0x1 bound_ctrl:0 ; encoding: [0xfa,0x02,0x00,0x7e,0x01,0x01,0x08,0x11]
define amdgpu_kernel void @dpp_test(i32 addrspace(1)* %out, i32 %in) {
  %tmp0 = call i32 @llvm.amdgcn.mov.dpp.i32(i32 %in, i32 1, i32 1, i32 1, i1 1) #0
  store i32 %tmp0, i32 addrspace(1)* %out
  ret void
}

; VI-LABEL: {{^}}dpp_wait_states:
; VI-NOOPT: v_mov_b32_e32 [[VGPR1:v[0-9]+]], s{{[0-9]+}}
; VI: v_mov_b32_e32 [[VGPR0:v[0-9]+]], s{{[0-9]+}}
; VI: s_nop 1
; VI-OPT: v_mov_b32_dpp [[VGPR0]], [[VGPR0]] quad_perm:[1,0,0,0] row_mask:0x1 bank_mask:0x1 bound_ctrl:0
; VI-NOOPT: v_mov_b32_dpp [[VGPR1]], [[VGPR0]] quad_perm:[1,0,0,0] row_mask:0x1 bank_mask:0x1 bound_ctrl:0
; VI: s_nop 1
; VI-OPT: v_mov_b32_dpp v{{[0-9]+}}, [[VGPR0]] quad_perm:[1,0,0,0] row_mask:0x1 bank_mask:0x1 bound_ctrl:0
; VI-NOOPT: v_mov_b32_dpp v{{[0-9]+}}, [[VGPR1]] quad_perm:[1,0,0,0] row_mask:0x1 bank_mask:0x1 bound_ctrl:0
define amdgpu_kernel void @dpp_wait_states(i32 addrspace(1)* %out, i32 %in) {
  %tmp0 = call i32 @llvm.amdgcn.mov.dpp.i32(i32 %in, i32 1, i32 1, i32 1, i1 1) #0
  %tmp1 = call i32 @llvm.amdgcn.mov.dpp.i32(i32 %tmp0, i32 1, i32 1, i32 1, i1 1) #0
  store i32 %tmp1, i32 addrspace(1)* %out
  ret void
}

; VI-LABEL: {{^}}dpp_first_in_bb:
; VI: ; %endif
; VI-OPT: s_mov_b32
; VI-OPT: s_mov_b32
; VI-NOOPT: s_nop 1
; VI: v_mov_b32_dpp [[VGPR0:v[0-9]+]], v{{[0-9]+}} quad_perm:[1,0,0,0] row_mask:0x1 bank_mask:0x1 bound_ctrl:0
; VI: s_nop 1
; VI: v_mov_b32_dpp [[VGPR1:v[0-9]+]], [[VGPR0]] quad_perm:[1,0,0,0] row_mask:0x1 bank_mask:0x1 bound_ctrl:0
; VI: s_nop 1
; VI: v_mov_b32_dpp v{{[0-9]+}}, [[VGPR1]] quad_perm:[1,0,0,0] row_mask:0x1 bank_mask:0x1 bound_ctrl:0
define amdgpu_kernel void @dpp_first_in_bb(float addrspace(1)* %out, float addrspace(1)* %in, float %cond, float %a, float %b) {
  %cmp = fcmp oeq float %cond, 0.0
  br i1 %cmp, label %if, label %else

if:
  %out_val = load float, float addrspace(1)* %out
  %if_val = fadd float %a, %out_val
  br label %endif

else:
  %in_val = load float, float addrspace(1)* %in
  %else_val = fadd float %b, %in_val
  br label %endif

endif:
  %val = phi float [%if_val, %if], [%else_val, %else]
  %val_i32 = bitcast float %val to i32
  %tmp0 = call i32 @llvm.amdgcn.mov.dpp.i32(i32 %val_i32, i32 1, i32 1, i32 1, i1 1) #0
  %tmp1 = call i32 @llvm.amdgcn.mov.dpp.i32(i32 %tmp0, i32 1, i32 1, i32 1, i1 1) #0
  %tmp2 = call i32 @llvm.amdgcn.mov.dpp.i32(i32 %tmp1, i32 1, i32 1, i32 1, i1 1) #0
  %tmp_float = bitcast i32 %tmp2 to float
  store float %tmp_float, float addrspace(1)* %out
  ret void
}

declare i32 @llvm.amdgcn.mov.dpp.i32(i32, i32, i32, i32, i1) #0

attributes #0 = { nounwind readnone convergent }

