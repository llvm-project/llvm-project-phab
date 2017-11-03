; RUN: llc -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=bonaire < %s | FileCheck -check-prefix=GCN -check-prefix=CI -check-prefix=ALL %s
; RUN: llc -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=carrizo -mattr=-flat-for-global < %s | FileCheck -check-prefix=GCN -check-prefix=VI -check-prefix=ALL %s
; RUN: llc -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=gfx900 -mattr=-flat-for-global < %s | FileCheck -check-prefix=GCN -check-prefix=GFX9 -check-prefix=ALL %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; ALL-LABEL: {{^}}large_alloca_pixel_shader:
; GCN-DAG: s_mov_b32 s8, SCRATCH_RSRC_DWORD0
; GCN-DAG: s_mov_b32 s9, SCRATCH_RSRC_DWORD1
; GCN-DAG: s_mov_b32 s10, -1
; CI-DAG: s_mov_b32 s11, 0xe8f000
; VI-DAG: s_mov_b32 s11, 0xe80000
; GFX9-DAG: s_mov_b32 s11, 0xe00000

; GCN: buffer_store_dword {{v[0-9]+}}, {{v[0-9]+}}, s[8:11], s0 offen
; GCN: buffer_load_dword {{v[0-9]+}}, {{v[0-9]+}}, s[8:11], s0 offen

; ALL: ; ScratchSize: 32772
define amdgpu_ps void @large_alloca_pixel_shader(i32 %x, i32 %y) #0 {
  %large = alloca [8192 x i32], align 4, addrspace(5)
  %gep = getelementptr [8192 x i32], [8192 x i32] addrspace(5)* %large, i32 0, i32 8191
  store volatile i32 %x, i32 addrspace(5)* %gep
  %gep1 = getelementptr [8192 x i32], [8192 x i32] addrspace(5)* %large, i32 0, i32 %y
  %val = load volatile i32, i32 addrspace(5)* %gep1
  store volatile i32 %val, i32 addrspace(1)* undef
  ret void
}

; ALL-LABEL: {{^}}large_alloca_pixel_shader_inreg:
; GCN-DAG: s_mov_b32 s8, SCRATCH_RSRC_DWORD0
; GCN-DAG: s_mov_b32 s9, SCRATCH_RSRC_DWORD1
; GCN-DAG: s_mov_b32 s10, -1
; CI-DAG: s_mov_b32 s11, 0xe8f000
; VI-DAG: s_mov_b32 s11, 0xe80000
; GFX9-DAG: s_mov_b32 s11, 0xe00000

; GCN: buffer_store_dword {{v[0-9]+}}, {{v[0-9]+}}, s[8:11], s2 offen
; GCN: buffer_load_dword {{v[0-9]+}}, {{v[0-9]+}}, s[8:11], s2 offen

; ALL: ; ScratchSize: 32772
define amdgpu_ps void @large_alloca_pixel_shader_inreg(i32 inreg %x, i32 inreg %y) #0 {
  %large = alloca [8192 x i32], align 4, addrspace(5)
  %gep = getelementptr [8192 x i32], [8192 x i32] addrspace(5)* %large, i32 0, i32 8191
  store volatile i32 %x, i32 addrspace(5)* %gep
  %gep1 = getelementptr [8192 x i32], [8192 x i32] addrspace(5)* %large, i32 0, i32 %y
  %val = load volatile i32, i32 addrspace(5)* %gep1
  store volatile i32 %val, i32 addrspace(1)* undef
  ret void
}

attributes #0 = { nounwind  }
