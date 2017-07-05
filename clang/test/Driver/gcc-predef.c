// Test that clang preincludes stdc-predef.h on Linux with gcc installations
//
// RUN: %clang %s -### -c 2>&1 \
// RUN: --gcc-toolchain="" --sysroot=%S/Inputs/stdc-predef \
// RUN: | FileCheck -check-prefix CHECK-PREDEF %s

// CHECK-PREDEF: "-include" "stdc-predef.h"
int i;
