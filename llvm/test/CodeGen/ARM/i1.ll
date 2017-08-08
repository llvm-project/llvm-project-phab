; RUN: llc -mtriple=arm-eabi %s -o - | FileCheck %s

define i32 @test1(i1 %c, i32 %x) {
; CHECK-LABEL: test1:
; CHECK:    mov r0, #0
; CHECK-NEXT:    cmp r0, #0
; CHECK-NEXT:    movne r0, #0
; CHECK-NEXT:    moveq r0, #1
; CHECK-NEXT:    mov pc, lr
entry:
  br i1 undef, label %t, label %f

t:
  ret i32 1

f:
  ret i32 0
}
