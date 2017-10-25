# REQUIRES: x86

# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t
# RUN: ld.lld %t -o %t2 --icf=all --eh-frame-hdr --verbose | FileCheck %s

## Check we have 2 equal FDEs with different size
## because of padding in input object.
# RUN: llvm-objdump %t --dwarf=frames | FileCheck %s --check-prefix=OBJ
# OBJ: .debug_frame contents:
# OBJ:    {{.*}} 00000010 {{.*}} FDE
# OBJ:    {{.*}} 00000014 {{.*}} FDE

## Check ICF merges sections with the same FDE data even when
## size is different because of padding bytes.
# CHECK:      selected .text.f1
# CHECK-NEXT:  removed .text.f2

.global personality
.hidden personality
personality:
 nop

.section .text.f1, "ax"
.cfi_startproc
.cfi_personality 0x1b, personality
nop
.cfi_endproc

.section .text.f2, "ax"
.cfi_startproc
.cfi_personality 0x1b, personality
nop
.cfi_endproc

