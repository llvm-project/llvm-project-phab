# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t
# RUN: ld.lld %t -o %t2 --icf=all --verbose | FileCheck %s
# CHECK: selected .text.foo
# CHECK:   removed .text.bar
# CHECK:   removed .order_bar

.section .text.foo, "ax"
nop

.section .text.bar, "ax"
nop

.section .order_foo,"ao",@progbits,.text.foo

.section .order_bar,"ao",@progbits,.text.bar
