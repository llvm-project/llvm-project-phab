// RUN: %clang_cc1 -flto=thin -emit-llvm-bc < %s | llvm-bcanalyzer -dump | FileCheck %s
// ; Check that the -flto=thin option emits a ThinLTO summary
// CHECK: <GLOBALVAL_SUMMARY_BLOCK
//
// RUN: %clang_cc1 -flto -triple x86_64-apple-darwin -emit-llvm-bc < %s | llvm-bcanalyzer -dump | FileCheck --check-prefix=LTO %s
// ; Check that we do not emit a summary for regular LTO on Apple platforms
// LTO-NOT: GLOBALVAL_SUMMARY_BLOCK
//
// RUN: %clang_cc1 -flto -triple x86_64-pc-linux-gnu -emit-llvm-bc < %s | llvm-bcanalyzer -dump | FileCheck --check-prefix=LTOINDEX %s
// ; Check that we emit a summary for regular LTO by default elsewhere
// LTOINDEX: <FULL_LTO_GLOBALVAL_SUMMARY_BLOCK
int main() {}
