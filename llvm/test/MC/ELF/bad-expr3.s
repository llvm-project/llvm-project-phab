// RUN: not llvm-mc -filetype=obj -triple x86_64-pc-linux-gnu %s -o /dev/null \
// RUN: 2>&1 | FileCheck %s

// CHECK: [[@LINE+1]]:{{[0-9]+}}: error: No relocation available to represent this relative expression
        .long foo - bar
        .section .zed
foo:
        .section .bah
bar:
