// RUN: llvm-mc -filetype=obj -triple x86_64-pc-linux-gnu %s -o - | llvm-readobj -s | FileCheck %s

// Test that we get optimal nops in text
    .text
f0:
    .align  0x100000000, 0x00000090
    .long 0

// CHECK:        Section {
// CHECK:          Name: .text
// CHECK-NEXT:     Type: SHT_PROGBITS
// CHECK-NEXT:     Flags [
// CHECK-NEXT:       SHF_ALLOC
// CHECK-NEXT:       SHF_EXECINSTR
// CHECK-NEXT:     ]
// CHECK-NEXT:     Address: 0x0
// CHECK-NEXT:     Offset: 0x100000000
// CHECK-NEXT:     Size: 4
// CHECK-NEXT:     Link: 0
// CHECK-NEXT:     Info: 0
// CHECK-NEXT:     AddressAlignment: 4294967296
// CHECK-NEXT:     EntrySize: 0
// CHECK-NEXT:   }

