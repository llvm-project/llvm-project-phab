// Test that clang preincludes stdc-predef.h, if the include file is available
//
// RUN: %clang %s -### -c 2>&1 \
// RUN: --sysroot=%S/Inputs/stdc-predef \
// RUN: | FileCheck -check-prefix CHECK-PREDEF %s
// RUN: %clang %s -c 2>&1 \
// RUN: --sysroot=%S/Inputs/basic_linux_tree \
// RUN: -DNOT_AVAIL=1 -Xclang -verify
// expected-no-diagnostics

// CHECK-PREDEF: "-fsystem-include-if-exists" "stdc-predef.h"
int i;
#if NOT_AVAIL
  /* In this test, the file stdc-predef.h is missing from the installation */
#if _STDC_PREDEF_H
  #error "stdc-predef.h should not be included"
#endif
#endif
