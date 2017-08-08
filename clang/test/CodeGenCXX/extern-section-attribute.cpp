// RUN: %clang_cc1 -emit-llvm %s -o - -ffreestanding -triple=i386-pc-linux-gnu | FileCheck %s --check-prefix=CHECK-LIN
// RUN: %clang_cc1 -emit-llvm %s -o - -ffreestanding -triple=x86_64-pc-linux-gnu | FileCheck %s --check-prefix=CHECK-LIN
// RUN: %clang_cc1 -emit-llvm %s -o - -ffreestanding -triple=i386-pc-win32 | FileCheck %s --check-prefix=CHECK-WIN
// RUN: %clang_cc1 -emit-llvm %s -o - -ffreestanding -triple=x86_64-pc-win32 | FileCheck %s  --check-prefix=CHECK-WIN

extern int aa __attribute__((section(".sdata")));
// CHECK-LIN-DAG: @aa = external global i32, section ".sdata", align 4
// CHECK-WIN-DAG: @"\01?aa@@3HA" = external global i32, section ".sdata", align 4

extern int bb __attribute__((section(".sdata"))) = 1;
// CHECK-LIN-DAG: @bb = global i32 1, section ".sdata", align 4
// CHECK-WIN-DAG: @"\01?bb@@3HA" = global i32 1, section ".sdata", align 4

int foo() {
  return aa + bb;
}
