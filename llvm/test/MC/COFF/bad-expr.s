// RUN: not llvm-mc -filetype=obj -triple i386-pc-win32 %s 2>&1 | FileCheck %s

        .data
_x:
// CHECK: [[@LINE+1]]:{{[0-9]+}}: error: No relocation available to represent this relative expression
        .long   _x-__ImageBase
