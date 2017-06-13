// RUN: %clang_cc1 -flto=thin -emit-llvm-bc < %s | llvm-bcanalyzer -dump | FileCheck %s
// ; Check that the -flto=thin option emits a ThinLTO summary
// CHECK: <GLOBALVAL_SUMMARY_BLOCK
//
// RUN: %clang_cc1 -flto -emit-llvm-bc < %s | llvm-bcanalyzer -dump | FileCheck --check-prefix=LTO %s
// ; Check that we do not emit a summary by default for regular LTO
// LTO-NOT: GLOBALVAL_SUMMARY_BLOCK
//
// RUN: %clang -flto -femit-summary-index -o - -c %s | llvm-bcanalyzer -dump | FileCheck --check-prefix=LTOINDEX %s
// ; Check that -femit-summary-index creates a summary with regular LTO
// RUN: %clang -emit-llvm -femit-summary-index -o - -c %s | llvm-bcanalyzer -dump | FileCheck --check-prefix=LTOINDEX %s
// ; Check that -femit-summary-index creates a summary with -emit-llvm
// LTOINDEX: <FULL_LTO_GLOBALVAL_SUMMARY_BLOCK
int main() {}
