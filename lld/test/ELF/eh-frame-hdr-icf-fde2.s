# REQUIRES: x86

# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t
# RUN: ld.lld %t -o %t2 --icf=all --eh-frame-hdr --verbose | FileCheck %s

## Check ICF does not merge sections with different FDE data.
# CHECK-NOT: selected

## Check we have 2 different FDEs in output.
# RUN: llvm-objdump %t2 --dwarf=frames | FileCheck %s --check-prefix=FRAMES
# FRAMES: .eh_frame contents:
# FRAMES:   00000018 00000014 0000001c FDE cie=0000001c pc=00000e68...00000e69
# FRAMES:   00000030 00000014 00000034 FDE cie=00000034 pc=00000e51...00000e52

.section .text.f1, "ax"
.cfi_startproc
nop
.cfi_endproc

.section .text.f2, "ax"
.cfi_startproc
.cfi_adjust_cfa_offset 8
nop
.cfi_endproc
