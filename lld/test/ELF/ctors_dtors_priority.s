// RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t1
// RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux \
// RUN:   %p/Inputs/ctors_dtors_priority1.s -o %t-crtbegin.o
// RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux \
// RUN:   %p/Inputs/ctors_dtors_priority2.s -o %t2
// RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux \
// RUN:   %p/Inputs/ctors_dtors_priority3.s -o %t-crtend.o
// RUN: ld.lld %t1 %t2 %t-crtend.o %t-crtbegin.o -o %t.exe
// RUN: llvm-objdump -s %t.exe | FileCheck %s
// REQUIRES: x86

.globl _start
_start:
  nop

.section .ctors, "aw", @progbits
  .quad 1
.section .ctors.100, "aw", @progbits
  .quad 2
.section .ctors.005, "aw", @progbits
  .quad 3
.section .ctors, "aw", @progbits
  .quad 4
.section .ctors, "aw", @progbits
  .quad 5

.section .dtors, "aw", @progbits
  .quad 0x11
.section .dtors.100, "aw", @progbits
  .quad 0x12
.section .dtors.005, "aw", @progbits
  .quad 0x13
.section .dtors, "aw", @progbits
  .quad 0x14
.section .dtors, "aw", @progbits
  .quad 0x15

// CHECK:      Contents of section .init_array:
// CHECK-NEXT: 200120 02000000 00000000 03000000 00000000
// CHECK-NEXT: 200130 05000000 00000000 04000000 00000000
// CHECK-NEXT: 200140 01000000 00000000 b1000000 00000000
// CHECK-NEXT: 200150 c1000000 00000000 a1000000 00000000
// CHECK-NEXT: Contents of section .fini_array:
// CHECK-NEXT: 200160 12000000 00000000 13000000 00000000
// CHECK-NEXT: 200170 15000000 00000000 14000000 00000000
// CHECK-NEXT: 200180 11000000 00000000 b2000000 00000000
// CHECK-NEXT: 200190 c2000000 00000000 a2000000 00000000
