; RUN: llc -mtriple=powerpc64-unknown-linux-gnu < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-unknown-linux-gnu -ppc-asm-full-reg-names < %s | FileCheck -check-prefix=CHECK-FN %s

define i64 @test1(i64 %a, i64 %b) {
; CHECK-LABEL: @test1
; CHECK-FN-LABEL: @test1

entry:
  ret i64 %b

; CHECK: mr 3, 4
; CHECK-FN: mr %r3, %r4

; CHECK: blr
; CHECK-FN: blr
}

define i64 @test2(i64 %a, i64 %b) {
; CHECK-LABEL: @test2
; CHECK-FN-LABEL: @test2

entry:
  ret i64 0

; CHECK: li 3, 0
; CHECK-FN: li %r3, 0

; CHECK: blr
; CHECK-FN: blr
}

