# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t.o
# RUN: echo "SECTIONS { . = 0x10000;                         \
# RUN:         symSize1 = SIZEOF(bar); symAddr1 = ADDR(bar); \
# RUN:         bar : ONLY_IF_RO { *(unknown) }               \
# RUN:         bar : ONLY_IF_RW { *(foo_aw) }                \
# RUN:         symSize2 = SIZEOF(bar); symAddr2 = ADDR(bar); \
# RUN:       }" > %t.script
# RUN: ld.lld -o %t -T %t.script %t.o
# RUN: llvm-readobj -s -t %t | FileCheck %s

## Check values of symbols are (1) not zeroes and (2) corresponds to
## size and address of bar that contains foo_aw input section.
# CHECK: Sections [
# CHECK:      Name: bar
# CHECK-NEXT: Type: SHT_PROGBITS
# CHECK-NEXT: Flags [
# CHECK-NEXT:   SHF_ALLOC
# CHECK-NEXT:   SHF_WRITE
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x10001
# CHECK-NEXT: Offset:
# CHECK-NEXT: Size: 8
# CHECK:      Name: symSize1
# CHECK-NEXT:  Value: 0x8
# CHECK:      Name: symAddr1
# CHECK-NEXT:  Value: 0x10001
# CHECK:      Name: symSize2
# CHECK-NEXT:  Value: 0x8
# CHECK:      Name: symAddr2
# CHECK-NEXT:  Value: 0x10001

.section foo_a,"a"
.byte 0

.section foo_aw,"aw"
.quad 0
