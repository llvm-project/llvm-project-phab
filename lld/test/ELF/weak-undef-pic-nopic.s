# REQUIRES: x86
# RUN: echo | llvm-mc -filetype=obj -triple=x86_64-pc-linux -o %t1.o -
# RUN: ld.lld -shared -o %t2.so %t1.o

# RUN: llvm-mc -filetype=obj -triple=x86_64-pc-linux -position-independent %s -o %t2.o
# RUN: ld.lld -pie -o %t2.exe %t2.o %t2.so
# RUN: llvm-readobj -dyn-symbols %t2.exe | FileCheck -check-prefix=PIC %s

# RUN: ld.lld -o %t3.exe %t2.o %t2.so
# RUN: llvm-readobj -dyn-symbols %t3.exe | FileCheck -check-prefix=NOPIC %s

# PIC:      Name: foo
# PIC-NEXT: Value: 0x0
# PIC-NEXT: Size: 0
# PIC-NEXT: Binding: Weak
# PIC-NEXT: Type: None
# PIC-NEXT: Other: 0
# PIC-NEXT: Section: Undefined

# NOPIC-NOT: Name: foo

.weak foo
