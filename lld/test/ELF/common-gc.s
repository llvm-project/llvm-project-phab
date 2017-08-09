# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-pc-linux %s -o %t

# RUN: ld.lld %t -o %t2
# RUN: llvm-readobj -sections -symbols %t2 | FileCheck %s --check-prefix=NOGC
# NOGC: Section {
# NOGC:   Index:
# NOGC:   Name: .bss
# NOGC-NEXT:   Type: SHT_NOBITS
# NOGC-NEXT:   Flags [
# NOGC-NEXT:     SHF_ALLOC
# NOGC-NEXT:     SHF_WRITE
# NOGC-NEXT:   ]
# NOGC-NEXT:   Address:
# NOGC-NEXT:   Offset:
# NOGC-NEXT:   Size: 8
# NOGC-NEXT:   Link:
# NOGC-NEXT:   Info:
# NOGC-NEXT:   AddressAlignment:
# NOGC-NEXT:   EntrySize:
# NOGC-NEXT: }
# NOGC:     Symbols [
# NOGC:       Name: bar
# NOGC:       Name: foo

# RUN: ld.lld -gc-sections %t -o %t1
# RUN: llvm-readobj -sections -symbols %t1 | FileCheck %s --check-prefix=GC
# GC: Section {
# GC:   Index:
# GC:   Name: .bss
# GC-NEXT:   Type: SHT_NOBITS
# GC-NEXT:   Flags [
# GC-NEXT:     SHF_ALLOC
# GC-NEXT:     SHF_WRITE
# GC-NEXT:   ]
# GC-NEXT:   Address:
# GC-NEXT:   Offset:
# GC-NEXT:   Size: 4
# GC-NEXT:   Link:
# GC-NEXT:   Info:
# GC-NEXT:   AddressAlignment:
# GC-NEXT:   EntrySize:
# GC-NEXT: }
# GC:     Symbols [
# GC-NOT:   Name: bar

.comm foo,4,4
.comm bar,4,4

.text
.globl _start
_start:
 .quad foo
