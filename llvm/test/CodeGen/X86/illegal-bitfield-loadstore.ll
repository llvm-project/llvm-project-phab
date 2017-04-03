; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown-linux-gnu -mcpu=corei7 | FileCheck %s

define void @i24_or(i24* %a) {
; CHECK-LABEL: i24_or:
; CHECK:       # BB#0:
; CHECK-NEXT:    orw $384, (%rdi) # imm = 0x180
; CHECK-NEXT:    retq
  %aa = load i24, i24* %a, align 1
  %b = or i24 %aa, 384
  store i24 %b, i24* %a, align 1
  ret void
}

define void @i24_and_or(i24* %a) {
; CHECK-LABEL: i24_and_or:
; CHECK:       # BB#0:
; CHECK-NEXT:    movzwl (%rdi), %eax
; CHECK-NEXT:    orl $384, %eax # imm = 0x180
; CHECK-NEXT:    andl $65408, %eax # imm = 0xFF80
; CHECK-NEXT:    movw %ax, (%rdi)
; CHECK-NEXT:    retq
  %b = load i24, i24* %a, align 1
  %c = and i24 %b, -128
  %d = or i24 %c, 384
  store i24 %d, i24* %a, align 1
  ret void
}

define void @i24_insert_bit(i24* %a, i1 zeroext %bit) {
; CHECK-LABEL: i24_insert_bit:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb 1(%rdi), %al
; CHECK-NEXT:    shlb $5, %sil
; CHECK-NEXT:    andb $-33, %al
; CHECK-NEXT:    orb %sil, %al
; CHECK-NEXT:    movb %al, 1(%rdi)
; CHECK-NEXT:    retq
  %extbit = zext i1 %bit to i24
  %b = load i24, i24* %a, align 1
  %extbit.shl = shl nuw nsw i24 %extbit, 13
  %c = and i24 %b, -8193
  %d = or i24 %c, %extbit.shl
  store i24 %d, i24* %a, align 1
  ret void
}

define void @i56_or(i56* %a) {
; CHECK-LABEL: i56_or:
; CHECK:       # BB#0:
; CHECK-NEXT:    orw $384, (%rdi) # imm = 0x180
; CHECK-NEXT:    retq
  %aa = load i56, i56* %a, align 1
  %b = or i56 %aa, 384
  store i56 %b, i56* %a, align 1
  ret void
}

define void @i56_and_or(i56* %a) {
; CHECK-LABEL: i56_and_or:
; CHECK:       # BB#0:
; CHECK-NEXT:    movzwl (%rdi), %eax
; CHECK-NEXT:    orl $384, %eax # imm = 0x180
; CHECK-NEXT:    andl $65408, %eax # imm = 0xFF80
; CHECK-NEXT:    movw %ax, (%rdi)
; CHECK-NEXT:    retq
  %b = load i56, i56* %a, align 1
  %c = and i56 %b, -128
  %d = or i56 %c, 384
  store i56 %d, i56* %a, align 1
  ret void
}

define void @i56_insert_bit(i56* %a, i1 zeroext %bit) {
; CHECK-LABEL: i56_insert_bit:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb 1(%rdi), %al
; CHECK-NEXT:    shlb $5, %sil
; CHECK-NEXT:    andb $-33, %al
; CHECK-NEXT:    orb %sil, %al
; CHECK-NEXT:    movb %al, 1(%rdi)
; CHECK-NEXT:    retq
  %extbit = zext i1 %bit to i56
  %b = load i56, i56* %a, align 1
  %extbit.shl = shl nuw nsw i56 %extbit, 13
  %c = and i56 %b, -8193
  %d = or i56 %c, %extbit.shl
  store i56 %d, i56* %a, align 1
  ret void
}

