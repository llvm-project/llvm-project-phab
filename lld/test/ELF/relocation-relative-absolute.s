# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-pc-linux %s -o %tinput1.o
# RUN: llvm-mc -filetype=obj -triple=x86_64-pc-linux \
# RUN:   %S/Inputs/relocation-relative-absolute.s -o %tinput2.o
# RUN: ld.lld %tinput1.o %tinput2.o -o %t -pie
# RUN: llvm-readobj -dyn-relocations %t | FileCheck %s

# CHECK:      Dynamic Relocations {
# CHECK-NEXT: }

.globl _start
_start:

call answer@PLT
