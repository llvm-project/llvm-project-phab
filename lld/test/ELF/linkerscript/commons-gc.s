# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t
# RUN: echo "SECTIONS { . = SIZEOF_HEADERS; foo = 1; }" > %t.script
# RUN: ld.lld %t --gc-sections --script %t.script -o %t1
# RUN: llvm-readobj -symbols %t1 | FileCheck %s

# CHECK:     foo
# CHECK-NOT: bar

.comm foo,4,4
.comm bar,4,4
