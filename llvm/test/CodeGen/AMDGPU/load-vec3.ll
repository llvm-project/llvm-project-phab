; RUN: llc -march=amdgcn -mcpu=tonga -mattr=+flat-for-global < %s | FileCheck -check-prefix=GCN -check-prefix=VI %s
; RUN: llc -march=amdgcn -mcpu=bonaire -mattr=-flat-for-global < %s | FileCheck -check-prefix=GCN -check-prefix=GCN-MUBUF %s
; RUN: llc -march=amdgcn -mcpu=gfx901 < %s | FileCheck -check-prefix=GCN -check-prefix=GFX9 %s

; GCN-LABEL: {{^}}load_global_v3i32:
; VI:   flat_load_dwordx3
; GFX9: global_load_dwordx3
; GCN-MUBUF-DAG: buffer_load_dwordx2 v
; GCN-MUBUF-DAG: buffer_load_dword v
define amdgpu_kernel void @load_global_v3i32(float addrspace(1)* nocapture readonly %in, <3 x float> addrspace(1)* nocapture %out) {
  %id = tail call i32 @llvm.amdgcn.workitem.id.x()
  %gep_in = getelementptr inbounds float, float addrspace(1)* %in, i32 %id
  %gep_in_v3 = bitcast float addrspace(1)* %gep_in to <3 x i32> addrspace(1)*
  %load = load <3 x i32>, <3 x i32> addrspace(1)* %gep_in_v3, align 4
  %gep_out = getelementptr inbounds <3 x float>, <3 x float> addrspace(1)* %out, i32 %id
  %vec3i = bitcast <3 x i32> %load to <3 x float>
  store <3 x float> %vec3i, <3 x float> addrspace(1)* %gep_out, align 16
  ret void
}

; GCN-LABEL: {{^}}load_global_v3f32:
; VI:   flat_load_dwordx3
; GFX9: global_load_dwordx3
; GCN-MUBUF-DAG: buffer_load_dwordx2 v
; GCN-MUBUF-DAG: buffer_load_dword v
define amdgpu_kernel void @load_global_v3f32(float addrspace(1)* nocapture readonly %in, <3 x float> addrspace(1)* nocapture %out) {
  %id = tail call i32 @llvm.amdgcn.workitem.id.x()
  %gep_in = getelementptr inbounds float, float addrspace(1)* %in, i32 %id
  %gep_in_v3 = bitcast float addrspace(1)* %gep_in to <3 x float> addrspace(1)*
  %load = load <3 x float>, <3 x float> addrspace(1)* %gep_in_v3, align 4
  %val = fadd <3 x float> %load, %load
  %gep_out = getelementptr inbounds <3 x float>, <3 x float> addrspace(1)* %out, i32 %id
  store <3 x float> %val, <3 x float> addrspace(1)* %gep_out, align 16
  ret void
}

; GCN-LABEL: {{^}}load_constant_v3i32:
; VI:   flat_load_dwordx3
; GFX9: global_load_dwordx3
; GCN-MUBUF-DAG: buffer_load_dwordx2 v
; GCN-MUBUF-DAG: buffer_load_dword v
define amdgpu_kernel void @load_constant_v3i32(i32 addrspace(2)* nocapture readonly %in, <3 x i32> addrspace(1)* nocapture %out) {
  %id = tail call i32 @llvm.amdgcn.workitem.id.x()
  %gep_in = getelementptr inbounds i32, i32 addrspace(2)* %in, i32 %id
  %gep_in_v3 = bitcast i32 addrspace(2)* %gep_in to <3 x i32> addrspace(2)*
  %load = load <3 x i32>, <3 x i32> addrspace(2)* %gep_in_v3, align 4
  %gep_out = getelementptr inbounds <3 x i32>, <3 x i32> addrspace(1)* %out, i32 %id
  store <3 x i32> %load, <3 x i32> addrspace(1)* %gep_out, align 16
  ret void
}

; GCN-LABEL: {{^}}load_flat_v3i32:
; GCN: flat_load_dwordx3
define amdgpu_kernel void @load_flat_v3i32(i32 addrspace(4)* nocapture readonly %in, <3 x i32> addrspace(1)* nocapture %out) {
  %id = tail call i32 @llvm.amdgcn.workitem.id.x()
  %gep_in = getelementptr inbounds i32, i32 addrspace(4)* %in, i32 %id
  %gep_in_v3 = bitcast i32 addrspace(4)* %gep_in to <3 x i32> addrspace(4)*
  %load = load <3 x i32>, <3 x i32> addrspace(4)* %gep_in_v3, align 4
  %gep_out = getelementptr inbounds <3 x i32>, <3 x i32> addrspace(1)* %out, i32 %id
  store <3 x i32> %load, <3 x i32> addrspace(1)* %gep_out, align 16
  ret void
}

; GCN-LABEL: {{^}}load_global_v3f16:
; GCN: {{buffer|flat|global}}_load_ushort v
; GCN: {{buffer|flat|global}}_load_ushort v
; GCN: {{buffer|flat|global}}_load_ushort v
; GCN-NOT: load_dwordx3
define amdgpu_kernel void @load_global_v3f16(half addrspace(1)* nocapture readonly %in, <3 x half> addrspace(1)* nocapture %out) {
  %id = tail call i32 @llvm.amdgcn.workitem.id.x()
  %gep_in = getelementptr inbounds half, half addrspace(1)* %in, i32 %id
  %gep_in_v3 = bitcast half addrspace(1)* %gep_in to <3 x half> addrspace(1)*
  %load = load <3 x half>, <3 x half> addrspace(1)* %gep_in_v3, align 2
  %val = fadd <3 x half> %load, %load
  %gep_out = getelementptr inbounds <3 x half>, <3 x half> addrspace(1)* %out, i32 %id
  store <3 x half> %val, <3 x half> addrspace(1)* %gep_out, align 8
  ret void
}

; GCN-LABEL: {{^}}load_global_v3i16_to_v3i32:
; GCN: {{buffer|flat|global}}_load_ushort v
; GCN: {{buffer|flat|global}}_load_ushort v
; GCN: {{buffer|flat|global}}_load_ushort v
; GCN-NOT: load_dwordx3
define amdgpu_kernel void @load_global_v3i16_to_v3i32(i16 addrspace(1)* nocapture readonly %in, <3 x i32> addrspace(1)* nocapture %out) {
  %id = tail call i32 @llvm.amdgcn.workitem.id.x()
  %gep_in = getelementptr inbounds i16, i16 addrspace(1)* %in, i32 %id
  %gep_in_v3 = bitcast i16 addrspace(1)* %gep_in to <3 x i16> addrspace(1)*
  %load = load <3 x i16>, <3 x i16> addrspace(1)* %gep_in_v3, align 2
  %val = zext <3 x i16> %load to <3 x i32>
  %gep_out = getelementptr inbounds <3 x i32>, <3 x i32> addrspace(1)* %out, i32 %id
  store <3 x i32> %val, <3 x i32> addrspace(1)* %gep_out, align 8
  ret void
}

; GCN-LABEL: {{^}}load_global_v3i32_scalar:
; GCN-DAG: s_load_dwordx2 s[{{[0-9:]+}}], s[{{[0-9:]+}}], 0x0
; GCN-DAG: s_load_dword s{{[0-9]+}}, s[{{[0-9:]+}}], 0x{{2|8}}
; GCN-NOT: load_dwordx3
define amdgpu_kernel void @load_global_v3i32_scalar(float addrspace(1)* nocapture readonly %in, <3 x i32> addrspace(1)* nocapture %out) {
  %gep_in = getelementptr inbounds float, float addrspace(1)* %in, i32 0
  %gep_in_v3 = bitcast float addrspace(1)* %gep_in to <3 x i32> addrspace(1)*
  %load = load <3 x i32>, <3 x i32> addrspace(1)* %gep_in_v3, align 4
  store <3 x i32> %load, <3 x i32> addrspace(1)* %out, align 16
  ret void
}

declare i32 @llvm.amdgcn.workitem.id.x() #1

attributes #1 = { nounwind readnone speculatable }
