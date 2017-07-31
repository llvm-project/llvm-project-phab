// Test that -ffreestanding suppresses the preinclude of stdc-predef.h.
//
// RUN: %clang %s -### -c -ffreestanding 2>&1 \
// RUN: --sysroot=%S/Inputs/stdc-predef \
// RUN: | FileCheck --implicit-check-not '"-finclude-if-exists" "stdc-predef.h"' %s

int i;
