# REQUIRES: x86

# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %tinput1.o
# RUN: llvm-mc -filetype=obj -triple=x86_64-pc-linux \
# RUN:   %S/Inputs/symbol-absolute.s -o %tinput2.o
# RUN: ld.lld -shared --gc-sections -o %t %tinput1.o %tinput2.o
# RUN: llvm-readobj --elf-output-style=GNU --file-headers --symbols %t | FileCheck %s
# CHECK: 0000000000000000     0 NOTYPE  LOCAL  HIDDEN   ABS _BASE

.text
.globl _start
_start:
       lea _BASE(%rip),%rax
