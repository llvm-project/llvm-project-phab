// REQUIRES: x86
// RUN: llvm-mc -filetype=obj -triple=x86_64-pc-linux %s -o %t.o
// RUN: ld.lld %t.o -o %t.so -shared -O2
// RUN: llvm-readobj -r -s %t.so | FileCheck %s

.section foo,"aMS",@progbits,1
.asciz "foobarbaz"
.asciz "barbaz"

.data
.quad foo + 11

// CHECK:      Name: foo
// CHECK-NEXT: Type: SHT_PROGBITS
// CHECK-NEXT: Flags [
// CHECK-NEXT:   SHF_ALLOC
// CHECK-NEXT:   SHF_MERGE
// CHECK-NEXT:   SHF_STRINGS
// CHECK-NEXT: ]
// CHECK-NEXT: Address: 0x1C8

// CHECK:      Relocations [
// CHECK-NEXT:   Section ({{.*}}) .rela.dyn {
// CHECK-NEXT:     0x{{.*}} R_X86_64_RELATIVE - 0x1CC
// CHECK-NEXT:   }
// CHECK-NEXT: ]
