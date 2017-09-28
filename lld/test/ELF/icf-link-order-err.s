# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t.o
# RUN: not ld.lld %t.o -o %t2 --icf=all 2>&1 | FileCheck %s
# CHECK: multiple SHF_LINK_ORDER sections are not supported: {{.*}}.o:(.text.bar)

.section .text.foo, "ax"
nop

.section .text.bar, "ax"
nop

.section .order_foo,"ao",@progbits,.text.foo
.quad 0

.section .order_bar1,"ao",@progbits,.text.bar
.quad 0

.section .order_bar2,"ao",@progbits,.text.bar
.quad 0
