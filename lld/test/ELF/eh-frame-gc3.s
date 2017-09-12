// REQUIRES: x86
// RUN: llvm-mc -filetype=obj -triple=i686-pc-linux %s -o %t.o
// RUN: ld.lld %t.o --gc-sections -o %tout
// RUN: llvm-nm %tout | FileCheck %s

// CHECK:     _start
// CHECK-NOT: foo
// CHECK-NOT: lsda
// CHECK-NOT: personality

        .global _start, foo, lsda, personality
        .text
_start:
        .cfi_startproc
        nop
        .cfi_endproc

        .section ".text.foo","ax"
foo:
        .cfi_startproc
        .cfi_personality 0, personality
        .cfi_lsda 0, .Lexception0
        nop
        .cfi_endproc

        .section ".lsda","a"
        .p2align 2
.Lexception0:
lsda:
        .byte 0

        .section ".text.personality","ax"
personality:
        nop
