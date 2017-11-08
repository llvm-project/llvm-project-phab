; RUN: llc < %s -mtriple=x86_64-linux -stack-size-section | FileCheck %s

; CHECK-LABEL: func1:
; CHECK: .section .stack_sizes,"",@progbits
; CHECK-NEXT: .quad func1
; CHECK-NEXT: .byte 8

; CHECK-LABEL: func2:
; CHECK: .section .stack_sizes,"",@progbits
; CHECK-NEXT: .quad func2
; CHECK-NEXT: .byte 24

define void @func1(i32, i32) #0 {
  alloca i32, align 4
  alloca i32, align 4
  ret void
}

define void @func2() #1 {
  alloca i32, align 4
  call void @func1(i32 1, i32 2)
  ret void
}

attributes #0 = { "no-frame-pointer-elim"="true" }
attributes #1 = { "no-frame-pointer-elim"="true" }
