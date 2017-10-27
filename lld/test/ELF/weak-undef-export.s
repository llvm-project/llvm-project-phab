# REQUIRES: x86

# Test that we resolve foo to zero at link-time.

# RUN: llvm-mc -filetype=obj -triple=x86_64-pc-linux %s -o %t.o
# RUN: ld.lld --export-dynamic %t.o -o %t

# RUN: llvm-readobj -symbols %t | FileCheck -check-prefix=SYM %s
# SYM:      Name: foo
# SYM-NEXT: Value: 0x0

# RUN: llvm-readobj -dyn-symbols %t | FileCheck -check-prefix=DYNSYM %s
# DYNSYM-NOT: foo

.weak foo
.quad foo
