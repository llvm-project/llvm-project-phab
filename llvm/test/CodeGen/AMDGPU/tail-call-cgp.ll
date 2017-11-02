; RUN: opt -S -mtriple=amdgcn-amd-amdhsa-amdgiz -codegenprepare %s | FileCheck %s

define internal fastcc void @callee(i32 addrspace(5)* nocapture %p, i32 %a) #0 {
  store volatile i32 %a, i32 addrspace(5)* %p, align 4
  ret void
}

; CHECK-LABEL: @func_caller(
; CHECK: tail call fastcc void @callee(
; CHECK-NEXT: ret void
; CHECK: ret void
define void @func_caller(i32 addrspace(5)* nocapture %p, i32 %a, i32 %b) #0 {
entry:
  %cmp = icmp eq i32 %b, 0
  br i1 %cmp, label %bb, label %ret

bb:
  tail call fastcc void @callee(i32 addrspace(5)* %p, i32 %a)
  br label %ret

ret:
  ret void
}

; CHECK-LABEL: @kernel_caller(
; CHECK: tail call fastcc void @callee(
; CHECK-NEXT: br label %ret

; CHECK: ret void
define amdgpu_kernel void @kernel_caller(i32 addrspace(5)* nocapture %p, i32 %a, i32 %b) #0 {
entry:
  %cmp = icmp eq i32 %b, 0
  br i1 %cmp, label %bb, label %ret

bb:
  tail call fastcc void @callee(i32 addrspace(5)* %p, i32 %a)
  br label %ret

ret:
  ret void
}

attributes #0 = { nounwind }
