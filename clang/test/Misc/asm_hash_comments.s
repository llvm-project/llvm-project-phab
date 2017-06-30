// RUN: %clang_cc1 -E -x assembler-with-cpp %s | FileCheck --strict-whitespace %s
// CHECK: # some comment 1
// CHECK-NEXT: # some comment 2
// CHECK-NEXT: # some comment 3

# some comment 1
# some comment 2
# some comment 3


