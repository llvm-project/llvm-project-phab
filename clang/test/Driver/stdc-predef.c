// Test that clang preincludes stdc-predef.h, if the include file is available
//
// RUN: %clang %s -### -c 2>&1 \
// RUN: --sysroot=%S/Inputs/stdc-predef \
// RUN: | FileCheck -check-prefix CHECK-PREDEF %s
// RUN: %clang %s -c -E 2>&1 \
// RUN: --sysroot=%S/Inputs/basic_linux_tree \
// RUN: | FileCheck --implicit-check-not "stdc-predef.h" %s
// expected-no-diagnostics

// CHECK-PREDEF: "-fsystem-include-if-exists" "stdc-predef.h"
int i;
