; RUN: not llc -march=amdgcn -mtriple=amdgcn---amdgiz < %s 2>&1 | FileCheck %s
target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"

; FIXME: Error is misleading because it's not an indirect call.

; CHECK: error: <unknown>:0:0: in function crash_call_constexpr_cast void (): unsupported indirect call to function foo

; Make sure that AMDGPUPromoteAlloca doesn't crash if the called
; function is a constantexpr cast of a function.

declare void @foo(float addrspace(5)*) #0
declare void @foo.varargs(...) #0

; XCHECK: in function crash_call_constexpr_cast{{.*}}: unsupported call to function foo
define amdgpu_kernel void @crash_call_constexpr_cast() #0 {
  %alloca = alloca i32, addrspace(5)
  call void bitcast (void (float addrspace(5)*)* @foo to void (i32 addrspace(5)*)*)(i32 addrspace(5)* %alloca) #0
  ret void
}

; XCHECK: in function crash_call_constexpr_cast{{.*}}: unsupported call to function foo.varargs
define amdgpu_kernel void @crash_call_constexpr_cast_varargs() #0 {
  %alloca = alloca i32, addrspace(5)
  call void bitcast (void (...)* @foo.varargs to void (i32 addrspace(5)*)*)(i32 addrspace(5)* %alloca) #0
  ret void
}

attributes #0 = { nounwind }
