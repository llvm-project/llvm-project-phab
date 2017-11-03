; RUN: opt -mtriple=amdgcn---amdgiz -O1 -S -inline-threshold=1 -amdgpu-early-inline-all %s | FileCheck %s

@c_alias = alias i32 (i32), i32 (i32)* @callee

define i32 @callee(i32 %x) {
entry:
  %mul1 = mul i32 %x, %x
  %mul2 = mul i32 %mul1, %x
  %mul3 = mul i32 %mul1, %mul2
  %mul4 = mul i32 %mul3, %mul2
  %mul5 = mul i32 %mul4, %mul3
  ret i32 %mul5
}

; CHECK-LABEL: @caller
; CHECK: mul i32
; CHECK-NOT: call i32

; CHECK: define i32 @c_alias
define amdgpu_kernel void @caller(i32 %x) {
entry:
  %res = call i32 @callee(i32 %x)
  store volatile i32 %res, i32 addrspace(1)* undef
  ret void
}
