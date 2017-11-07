; RUN: llc < %s -O2 -verify-machineinstrs -mtriple=powerpc64le-unknown-linux-gnu | FileCheck %s

define i8* @func1(i8* %a, i1 %b) {
; CHECK-LABEL: @func1
; CHECK: bc
; CHECK: # BB#1
; CHECK: mr 30, 3
; CHECK: bl callee
; CHECK: mr 3, 30
; CHECK: .LBB0_2:
; CHECK:  blr
entry:
  br i1 %b, label %exit, label %foo

foo:
  call void @callee()
  br label %exit

exit:
  ret i8* %a
}

declare void @callee()
