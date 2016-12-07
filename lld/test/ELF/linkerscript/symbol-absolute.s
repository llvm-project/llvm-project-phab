# REQUIRES: x86

# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t
# RUN: echo "PROVIDE_HIDDEN(base = 0);" > %t.script
# RUN: ld.lld -shared --gc-sections --script %t.script -o %t1 %t
# RUN: llvm-readobj --elf-output-style=GNU --file-headers --symbols %t1 | FileCheck %s
# CHECK: 0000000000000000     0 NOTYPE  LOCAL  HIDDEN   ABS base

.text
.globl _start
_start:
	lea base(%rip),%rax
