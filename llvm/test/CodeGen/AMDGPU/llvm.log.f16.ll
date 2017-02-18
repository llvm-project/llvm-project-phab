; RUN: llc -march=amdgcn -verify-machineinstrs < %s | FileCheck -check-prefix=GCN -check-prefix=SI %s
; RUN: llc -march=amdgcn -mcpu=fiji -mattr=-flat-for-global -verify-machineinstrs < %s | FileCheck -check-prefix=GCN -check-prefix=VI %s

declare half @llvm.log.f16(half %a)
declare <2 x half> @llvm.log.v2f16(<2 x half> %a)

; GCN-LABEL: {{^}}log_f16
; GCN: buffer_load_ushort v[[A_F16_0:[0-9]+]]
; SI:  v_mov_b32_e32 v[[A_F32_1:[0-9]+]]
; SI:  v_cvt_f32_f16_e32 v[[A_F32_0:[0-9]+]], v[[A_F16_0]]
; SI:  v_log_f32_e32 v[[R_F32_0:[0-9]+]], v[[A_F32_0]]
; SI:  v_div_scale_f32 v[[A_F32_2:[0-9]+]], {{s\[[0-9]+:[0-9]+\]}}, v[[A_F32_1]], v[[A_F32_1]], v[[A_F32_0]]
; SI:  v_cvt_f16_f32_e32 v[[R_F16_0:[0-9]+]], v[[R_F32_0]]
; VI:  v_rcp_f32_e32 v[[A_F32_0:[0-9]+]]
; VI:  v_log_f16_e32 v[[R_F16_1:[0-9]+]], v[[A_F16_0]]
; VI:  v_cvt_f32_f16_e32 v[[R_F32_2:[0-9]+]], v[[R_F16_1]]
; VI:  v_mul_f32_e32 v[[R_F32_0:[0-9]+]], v[[A_F32_0]], v[[R_F32_2]]
; VI:  v_cvt_f16_f32_e32 v[[R_F16_0:[0-9]+]], v[[R_F32_0]]
; GCN: buffer_store_short v[[R_F16_0]]
; GCN: s_endpgm
define void @log_f16(
    half addrspace(1)* %r,
    half addrspace(1)* %a) {
entry:
  %a.val = load half, half addrspace(1)* %a
  %r.val = call half @llvm.log.f16(half %a.val)
  store half %r.val, half addrspace(1)* %r
  ret void
}

; GCN-LABEL: {{^}}log_v2f16
; GCN: buffer_load_dword v[[A_F16_0:[0-9]+]]
; SI:  v_mov_b32_e32 v[[A_F32_2:[0-9]+]]
; SI:  v_cvt_f32_f16_e32 v[[A_F32_1:[0-9]+]], v[[A_F16_0]]
; VI:  v_rcp_f32_e32 v[[A_F32_0:[0-9]+]]
; GCN: v_lshrrev_b32_e32 v[[R_F16_0:[0-9]+]], 16, v[[A_F16_0]]
; SI:  v_cvt_f32_f16_e32 v[[A_F32_0:[0-9]+]], v[[R_F16_0]]
; SI:  v_log_f32_e32 v[[R_F32_1:[0-9]+]], v[[A_F32_1]]
; SI:  v_div_scale_f32 v[[A_F32_3:[0-9]+]], {{s\[[0-9]+:[0-9]+\]}}, v[[A_F32_2]], v[[A_F32_2]], v[[R_F32_1]]
; SI:  v_log_f32_e32 v[[R_F32_0:[0-9]+]], v[[A_F32_0]]
; SI:  v_cvt_f16_f32_e32 v[[R_F16_1:[0-9]+]], v[[R_F32_1]]
; SI:  v_div_scale_f32 v[[A_F32_3:[0-9]+]], {{s\[[0-9]+:[0-9]+\]}}, v[[A_F32_2]], v[[A_F32_2]], v[[R_F32_0]]
; VI:  v_log_f16_e32 v[[R_F16_1:[0-9]+]], v[[A_F16_0]]
; VI:  v_log_f16_e32 v[[R_F16_2:[0-9]+]], v[[R_F16_0]]
; VI:  v_cvt_f32_f16_e32 v[[R_F32_3:[0-9]+]], v[[R_F16_1]]
; VI:  v_cvt_f32_f16_e32 v[[R_F32_4:[0-9]+]], v[[R_F16_2]]
; VI:  v_mul_f32_e32 v[[R_F32_3:[0-9]+]], v[[A_F32_0]], v[[R_F32_3]]
; VI:  v_mul_f32_e32 v[[R_F32_0:[0-9]+]], v[[A_F32_0]], v[[R_F32_4]]
; VI:  v_cvt_f16_f32_e32 v[[R_F16_3:[0-9]+]], v[[R_F32_3]]
; VI:  v_cvt_f16_f32_e32 v[[R_F16_0:[0-9]+]], v[[R_F32_0]]
; GCN: v_and_b32_e32 v[[R_F16_LO:[0-9]+]], 0xffff, v[[R_F16_1]]
; SI:  v_cvt_f16_f32_e32 v[[R_F16_0:[0-9]+]], v[[R_F32_0]]
; GCN: v_lshlrev_b32_e32 v[[R_F16_HI:[0-9]+]], 16, v[[R_F16_0]]
; GCN: v_or_b32_e32 v[[R_V2_F16:[0-9]+]], v[[R_F16_HI]], v[[R_F16_LO]]
; GCN: buffer_store_dword v[[R_V2_F16]]
; GCN: s_endpgm
define void @log_v2f16(
    <2 x half> addrspace(1)* %r,
    <2 x half> addrspace(1)* %a) {
entry:
  %a.val = load <2 x half>, <2 x half> addrspace(1)* %a
  %r.val = call <2 x half> @llvm.log.v2f16(<2 x half> %a.val)
  store <2 x half> %r.val, <2 x half> addrspace(1)* %r
  ret void
}
