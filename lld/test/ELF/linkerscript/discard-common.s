# REQUIRES: x86

# RUN: llvm-mc -triple x86_64-pc-linux %s -o %t.o -filetype=obj
# RUN: echo "SECTIONS { /DISCARD/ : { *(COMMON) } }" > %t.script
# RUN: ld.lld -o %t -T %t.script %t.o
# RUN: llvm-readobj -symbols -sections %t | FileCheck %s
# CHECK-NOT: .bss
# CHECK-NOT: foo

.comm foo,4,4
