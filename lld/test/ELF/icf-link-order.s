# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t
# RUN: ld.lld %t -o %t2 --icf=all --verbose | FileCheck %s
# CHECK:      selected .text.foo
# CHECK-NEXT:   removed .text.bar
# CHECK-NEXT:   removed .order_bar
# CHECK-NOT: selected
# CHECK-NOT: removed

.section .text.foo, "ax"
nop

.section .text.bar, "ax"
nop

.section .text.dah, "ax"
nop

.section .text.zed, "ax"
nop

.section .order_foo,"ao",@progbits,.text.foo
.quad 0

.section .order_bar,"ao",@progbits,.text.bar
.quad 0

.section .order_dah,"ao",@progbits,.text.dah
.quad 1
