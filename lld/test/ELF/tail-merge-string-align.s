// REQUIRES: x86
// RUN: llvm-mc -filetype=obj -triple=x86_64-pc-linux %s -o %t.o
// RUN: ld.lld %t.o -o %t.so -shared -O2
// RUN: llvm-readobj -s -section-data %t.so | FileCheck %s

.section        .rodata.4a,"aMS",@progbits,1
.align 4
.asciz "abcdefg"

.section        .rodata.4b,"aMS",@progbits,1
.align 4
.asciz "efg"

.section        .rodata.4c,"aMS",@progbits,1
.align 4
.asciz "defgh"

// CHECK:      Name: .rodata
// CHECK-NEXT: Type: SHT_PROGBITS
// CHECK-NEXT: Flags [
// CHECK-NEXT:   SHF_ALLOC
// CHECK-NEXT:   SHF_MERGE
// CHECK-NEXT:   SHF_STRINGS
// CHECK-NEXT: ]
// CHECK-NEXT: Address:
// CHECK-NEXT: Offset:
// CHECK-NEXT: Size:
// CHECK-NEXT: Link: 0
// CHECK-NEXT: Info: 0
// CHECK-NEXT: AddressAlignment: 4
// CHECK-NEXT: EntrySize:
// CHECK-NEXT: SectionData (
// CHECK-NEXT:   |abcdefg.defgh.|
// CHECK-NEXT: )
