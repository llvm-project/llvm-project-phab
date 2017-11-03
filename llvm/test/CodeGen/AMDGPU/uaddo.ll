; RUN:  llc -amdgpu-scalarize-global-loads=false  -march=amdgcn -mtriple=amdgcn---amdgiz -verify-machineinstrs < %s | FileCheck -check-prefixes=GCN,SI,FUNC %s
; RUN:  llc -amdgpu-scalarize-global-loads=false  -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tonga -verify-machineinstrs < %s | FileCheck -check-prefixes=GCN,VI,FUNC %s
; RUN:  llc -amdgpu-scalarize-global-loads=false  -march=r600 -mtriple=r600---amdgiz -mcpu=cypress -verify-machineinstrs < %s | FileCheck -check-prefixes=EG,FUNC %s

; FUNC-LABEL: {{^}}s_uaddo_i64_zext:
; GCN: s_add_u32
; GCN: s_addc_u32
; GCN: v_cmp_lt_u64_e32 vcc

; EG: ADDC_UINT
; EG: ADDC_UINT
define amdgpu_kernel void @s_uaddo_i64_zext(i64 addrspace(1)* %out, i64 %a, i64 %b) #0 {
  %uadd = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %a, i64 %b)
  %val = extractvalue { i64, i1 } %uadd, 0
  %carry = extractvalue { i64, i1 } %uadd, 1
  %ext = zext i1 %carry to i64
  %add2 = add i64 %val, %ext
  store i64 %add2, i64 addrspace(1)* %out, align 8
  ret void
}

; FIXME: Could do scalar

; FUNC-LABEL: {{^}}s_uaddo_i32:
; GCN: v_add_i32_e32 v{{[0-9]+}}, vcc, s{{[0-9]+}}, v{{[0-9]+}}
; GCN: v_cndmask_b32_e64 v{{[0-9]+}}, 0, 1, vcc

; EG: ADDC_UINT
; EG: ADD_INT
define amdgpu_kernel void @s_uaddo_i32(i32 addrspace(1)* %out, i1 addrspace(1)* %carryout, i32 %a, i32 %b) #0 {
  %uadd = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
  %val = extractvalue { i32, i1 } %uadd, 0
  %carry = extractvalue { i32, i1 } %uadd, 1
  store i32 %val, i32 addrspace(1)* %out, align 4
  store i1 %carry, i1 addrspace(1)* %carryout
  ret void
}

; FUNC-LABEL: {{^}}v_uaddo_i32:
; GCN: v_add_i32_e32 v{{[0-9]+}}, vcc, v{{[0-9]+}}, v{{[0-9]+}}
; GCN: v_cndmask_b32_e64 v{{[0-9]+}}, 0, 1, vcc

; EG: ADDC_UINT
; EG: ADD_INT
define amdgpu_kernel void @v_uaddo_i32(i32 addrspace(1)* %out, i1 addrspace(1)* %carryout, i32 addrspace(1)* %a.ptr, i32 addrspace(1)* %b.ptr) #0 {
  %tid = call i32 @llvm.amdgcn.workitem.id.x()
  %tid.ext = sext i32 %tid to i64
  %a.gep = getelementptr inbounds i32, i32 addrspace(1)* %a.ptr
  %b.gep = getelementptr inbounds i32, i32 addrspace(1)* %b.ptr
  %a = load i32, i32 addrspace(1)* %a.gep, align 4
  %b = load i32, i32 addrspace(1)* %b.gep, align 4
  %uadd = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
  %val = extractvalue { i32, i1 } %uadd, 0
  %carry = extractvalue { i32, i1 } %uadd, 1
  store i32 %val, i32 addrspace(1)* %out, align 4
  store i1 %carry, i1 addrspace(1)* %carryout
  ret void
}

; FUNC-LABEL: {{^}}v_uaddo_i32_novcc:
; GCN: v_add_i32_e32 v{{[0-9]+}}, vcc, v{{[0-9]+}}, v{{[0-9]+}}
; GCN: v_cndmask_b32_e64 v{{[0-9]+}}, 0, 1, vcc

; EG: ADDC_UINT
; EG: ADD_INT
define amdgpu_kernel void @v_uaddo_i32_novcc(i32 addrspace(1)* %out, i1 addrspace(1)* %carryout, i32 addrspace(1)* %a.ptr, i32 addrspace(1)* %b.ptr) #0 {
  %tid = call i32 @llvm.amdgcn.workitem.id.x()
  %tid.ext = sext i32 %tid to i64
  %a.gep = getelementptr inbounds i32, i32 addrspace(1)* %a.ptr
  %b.gep = getelementptr inbounds i32, i32 addrspace(1)* %b.ptr
  %a = load i32, i32 addrspace(1)* %a.gep, align 4
  %b = load i32, i32 addrspace(1)* %b.gep, align 4
  %uadd = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
  %val = extractvalue { i32, i1 } %uadd, 0
  %carry = extractvalue { i32, i1 } %uadd, 1
  store volatile i32 %val, i32 addrspace(1)* %out, align 4
  call void asm sideeffect "", "~{VCC}"() #0
  store volatile i1 %carry, i1 addrspace(1)* %carryout
  ret void
}

; FUNC-LABEL: {{^}}s_uaddo_i64:
; GCN: s_add_u32
; GCN: s_addc_u32

; EG: ADDC_UINT
; EG: ADD_INT
define amdgpu_kernel void @s_uaddo_i64(i64 addrspace(1)* %out, i1 addrspace(1)* %carryout, i64 %a, i64 %b) #0 {
  %uadd = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %a, i64 %b)
  %val = extractvalue { i64, i1 } %uadd, 0
  %carry = extractvalue { i64, i1 } %uadd, 1
  store i64 %val, i64 addrspace(1)* %out, align 8
  store i1 %carry, i1 addrspace(1)* %carryout
  ret void
}

; FUNC-LABEL: {{^}}v_uaddo_i64:
; GCN: v_add_i32
; GCN: v_addc_u32

; EG: ADDC_UINT
; EG: ADD_INT
define amdgpu_kernel void @v_uaddo_i64(i64 addrspace(1)* %out, i1 addrspace(1)* %carryout, i64 addrspace(1)* %a.ptr, i64 addrspace(1)* %b.ptr) #0 {
  %tid = call i32 @llvm.amdgcn.workitem.id.x()
  %tid.ext = sext i32 %tid to i64
  %a.gep = getelementptr inbounds i64, i64 addrspace(1)* %a.ptr
  %b.gep = getelementptr inbounds i64, i64 addrspace(1)* %b.ptr
  %a = load i64, i64 addrspace(1)* %a.gep
  %b = load i64, i64 addrspace(1)* %b.gep
  %uadd = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %a, i64 %b)
  %val = extractvalue { i64, i1 } %uadd, 0
  %carry = extractvalue { i64, i1 } %uadd, 1
  store i64 %val, i64 addrspace(1)* %out
  store i1 %carry, i1 addrspace(1)* %carryout
  ret void
}

; FUNC-LABEL: {{^}}v_uaddo_i16:
; VI: v_add_u16_e32
; VI: v_cmp_lt_u16_e32
define amdgpu_kernel void @v_uaddo_i16(i16 addrspace(1)* %out, i1 addrspace(1)* %carryout, i16 addrspace(1)* %a.ptr, i16 addrspace(1)* %b.ptr) #0 {
  %tid = call i32 @llvm.amdgcn.workitem.id.x()
  %tid.ext = sext i32 %tid to i64
  %a.gep = getelementptr inbounds i16, i16 addrspace(1)* %a.ptr
  %b.gep = getelementptr inbounds i16, i16 addrspace(1)* %b.ptr
  %a = load i16, i16 addrspace(1)* %a.gep
  %b = load i16, i16 addrspace(1)* %b.gep
  %uadd = call { i16, i1 } @llvm.uadd.with.overflow.i16(i16 %a, i16 %b)
  %val = extractvalue { i16, i1 } %uadd, 0
  %carry = extractvalue { i16, i1 } %uadd, 1
  store i16 %val, i16 addrspace(1)* %out
  store i1 %carry, i1 addrspace(1)* %carryout
  ret void
}

declare i32 @llvm.amdgcn.workitem.id.x() #1
declare { i16, i1 } @llvm.uadd.with.overflow.i16(i16, i16) #1
declare { i32, i1 } @llvm.uadd.with.overflow.i32(i32, i32) #1
declare { i64, i1 } @llvm.uadd.with.overflow.i64(i64, i64) #1

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }
