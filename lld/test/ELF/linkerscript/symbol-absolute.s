# REQUIRES: x86

# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t
# RUN: echo "PROVIDE_HIDDEN(_BASE = 0);" > %t.script
# RUN: ld.lld -shared --gc-sections -o %t1 %t %t.script
# RUN: llvm-readobj --elf-output-style=GNU --file-headers --symbols %t1 | FileCheck %s
# CHECK: 0000000000000000     0 NOTYPE  LOCAL  HIDDEN   ABS _BASE

.text
.globl _start
_start:
       lea _BASE(%rip),%rax
