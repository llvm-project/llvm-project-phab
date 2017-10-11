# REQUIRES: x86

# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t
# RUN: ld.lld %t -o %t2 --icf=all --verbose | FileCheck %s

## In this test each section has 2 FDEs attached.
## .text.f1 and .text.f3 has equal FDEs and second FDE of.text.f2 section
## is different. Check we merge .text.f1 and .text.f3 together.

# CHECK-NOT:  .text.f2
# CHECK:      selected .text.f1
# CHECK-NEXT:  removed .text.f3
# CHECK-NOT:  .text.f2

.section .text.f1, "ax"
.cfi_startproc
nop
.cfi_endproc
nop
nop
.cfi_startproc
nop
nop
nop
.cfi_endproc

.section .text.f2, "ax"
.cfi_startproc
nop
.cfi_endproc
nop
nop
.cfi_startproc
.cfi_adjust_cfa_offset 8
nop
nop
nop
.cfi_endproc

.section .text.f3, "ax"
.cfi_startproc
nop
.cfi_endproc
nop
nop
.cfi_startproc
nop
nop
nop
.cfi_endproc
