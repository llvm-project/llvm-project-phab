; RUN: llc -march=amdgcn -mtriple=amdgcn---amdgiz -mattr=-promote-alloca -amdgpu-sroa=0 -verify-machineinstrs < %s | FileCheck -check-prefix=SI -check-prefix=FUNC %s
; RUN: llc -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tonga -mattr=-promote-alloca -amdgpu-sroa=0 -verify-machineinstrs < %s | FileCheck -check-prefix=SI -check-prefix=FUNC %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; FUNC-LABEL: {{^}}load_i8_sext_private:
; SI: buffer_load_sbyte v{{[0-9]+}}, off, s[{{[0-9]+:[0-9]+}}], s{{[0-9]+}} offset:4{{$}}
define amdgpu_kernel void @load_i8_sext_private(i32 addrspace(1)* %out) {
entry:
  %tmp0 = alloca i8, addrspace(5)
  %tmp1 = load i8, i8 addrspace(5)* %tmp0
  %tmp2 = sext i8 %tmp1 to i32
  store i32 %tmp2, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}load_i8_zext_private:
; SI: buffer_load_ubyte v{{[0-9]+}}, off, s[{{[0-9]+:[0-9]+}}], s{{[0-9]+}} offset:4{{$}}
define amdgpu_kernel void @load_i8_zext_private(i32 addrspace(1)* %out) {
entry:
  %tmp0 = alloca i8, addrspace(5)
  %tmp1 = load i8, i8 addrspace(5)* %tmp0
  %tmp2 = zext i8 %tmp1 to i32
  store i32 %tmp2, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}load_i16_sext_private:
; SI: buffer_load_sshort v{{[0-9]+}}, off, s[{{[0-9]+:[0-9]+}}], s{{[0-9]+}} offset:4{{$}}
define amdgpu_kernel void @load_i16_sext_private(i32 addrspace(1)* %out) {
entry:
  %tmp0 = alloca i16, addrspace(5)
  %tmp1 = load i16, i16 addrspace(5)* %tmp0
  %tmp2 = sext i16 %tmp1 to i32
  store i32 %tmp2, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}load_i16_zext_private:
; SI: buffer_load_ushort v{{[0-9]+}}, off, s[{{[0-9]+:[0-9]+}}], s{{[0-9]+}} offset:4{{$}}
define amdgpu_kernel void @load_i16_zext_private(i32 addrspace(1)* %out) {
entry:
  %tmp0 = alloca i16, addrspace(5)
  %tmp1 = load volatile i16, i16 addrspace(5)* %tmp0
  %tmp2 = zext i16 %tmp1 to i32
  store i32 %tmp2, i32 addrspace(1)* %out
  ret void
}
