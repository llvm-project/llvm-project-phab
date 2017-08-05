# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t1
# RUN: ld.lld %t1 -e a -o %t -callgraph-ordering-file %p/Inputs/cgprofile.txt
# RUN: llvm-readobj -symbols %t | FileCheck %s

    .section .text.a,"ax",@progbits
    .global a
a:
    .zero 20

    .section .text.b,"ax",@progbits
    .global b
b:
    .zero 1
    
    .section .text.c,"ax",@progbits
    .global c
c:
    .zero 4095
    
    .section .text.d,"ax",@progbits
    .global d
d:
    .zero 51
    
    .section .text.e,"ax",@progbits
    .global e
e:
    .zero 42

    .section .text.f,"ax",@progbits
    .global f
f:
    .zero 42

# CHECK:     Symbols [
# CHECK-NEXT:  Symbol {
# CHECK-NEXT:    Name:  (0)
# CHECK-NEXT:    Value: 0x0
# CHECK-NEXT:    Size: 0
# CHECK-NEXT:    Binding: Local (0x0)
# CHECK-NEXT:    Type: None (0x0)
# CHECK-NEXT:    Other: 0
# CHECK-NEXT:    Section: Undefined (0x0)
# CHECK-NEXT:  }
# CHECK-NEXT:  Symbol {
# CHECK-NEXT:    Name: a
# CHECK-NEXT:    Value: 0x202000
# CHECK-NEXT:    Size: 0
# CHECK-NEXT:    Binding: Global (0x1)
# CHECK-NEXT:    Type: None (0x0)
# CHECK-NEXT:    Other: 0
# CHECK-NEXT:    Section: .text
# CHECK-NEXT:  }
# CHECK-NEXT:  Symbol {
# CHECK-NEXT:    Name: b
# CHECK-NEXT:    Value: 0x201FFF
# CHECK-NEXT:    Size: 0
# CHECK-NEXT:    Binding: Global (0x1)
# CHECK-NEXT:    Type: None (0x0)
# CHECK-NEXT:    Other: 0
# CHECK-NEXT:    Section: .text
# CHECK-NEXT:  }
# CHECK-NEXT:  Symbol {
# CHECK-NEXT:    Name: c
# CHECK-NEXT:    Value: 0x201000
# CHECK-NEXT:    Size: 0
# CHECK-NEXT:    Binding: Global (0x1)
# CHECK-NEXT:    Type: None (0x0)
# CHECK-NEXT:    Other: 0
# CHECK-NEXT:    Section: .text
# CHECK-NEXT:  }
# CHECK-NEXT:  Symbol {
# CHECK-NEXT:    Name: d
# CHECK-NEXT:    Value: 0x202068
# CHECK-NEXT:    Size: 0
# CHECK-NEXT:    Binding: Global (0x1)
# CHECK-NEXT:    Type: None (0x0)
# CHECK-NEXT:    Other: 0
# CHECK-NEXT:    Section: .text
# CHECK-NEXT:  }
# CHECK-NEXT:  Symbol {
# CHECK-NEXT:    Name: e
# CHECK-NEXT:    Value: 0x20203E
# CHECK-NEXT:    Size: 0
# CHECK-NEXT:    Binding: Global (0x1)
# CHECK-NEXT:    Type: None (0x0)
# CHECK-NEXT:    Other: 0
# CHECK-NEXT:    Section: .text
# CHECK-NEXT:  }
# CHECK-NEXT:  Symbol {
# CHECK-NEXT:    Name: f
# CHECK-NEXT:    Value: 0x202014
# CHECK-NEXT:    Size: 0
# CHECK-NEXT:    Binding: Global (0x1)
# CHECK-NEXT:    Type: None (0x0)
# CHECK-NEXT:    Other: 0
# CHECK-NEXT:    Section: .text
# CHECK-NEXT:  }
# CHECK-NEXT:]
