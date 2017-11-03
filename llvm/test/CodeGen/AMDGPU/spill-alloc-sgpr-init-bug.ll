; RUN: llc -march=amdgcn -mtriple=amdgcn---amdgiz -mcpu=tonga -verify-machineinstrs < %s | FileCheck --check-prefix=TONGA %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; On Tonga and Iceland, limited SGPR availability means care must be taken to
; allocate scratch registers correctly. Check that this test compiles without
; error.
; TONGA-LABEL: test
define amdgpu_kernel void @test(<256 x i32> addrspace(1)* %out, <256 x i32> addrspace(1)* %in) {
entry:
  %mbcnt.lo = call i32 @llvm.amdgcn.mbcnt.lo(i32 -1, i32 0)
  %tid = call i32 @llvm.amdgcn.mbcnt.hi(i32 -1, i32 %mbcnt.lo)
  %aptr = getelementptr <256 x i32>, <256 x i32> addrspace(1)* %in, i32 %tid
  %a = load <256 x i32>, <256 x i32> addrspace(1)* %aptr
  call void asm sideeffect "", "~{memory}" ()
  %outptr = getelementptr <256 x i32>, <256 x i32> addrspace(1)* %in, i32 %tid
  store <256 x i32> %a, <256 x i32> addrspace(1)* %outptr

; mark 128-bit SGPR registers as used so they are unavailable for the
; scratch resource descriptor
  call void asm sideeffect "", "~{SGPR4},~{SGPR8},~{SGPR12},~{SGPR16},~{SGPR20},~{SGPR24},~{SGPR28}" ()
  call void asm sideeffect "", "~{SGPR32},~{SGPR36},~{SGPR40},~{SGPR44},~{SGPR48},~{SGPR52},~{SGPR56}" ()
  call void asm sideeffect "", "~{SGPR60},~{SGPR64},~{SGPR68}" ()
  ret void
}

declare i32 @llvm.amdgcn.mbcnt.lo(i32, i32) #0
declare i32 @llvm.amdgcn.mbcnt.hi(i32, i32) #0

attributes #0 = { nounwind readnone }
