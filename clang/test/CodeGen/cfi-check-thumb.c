// Test that __cfi_check and __cfi_check_fail have common attributes and calling convention.
// Also __cfi_check is always using Thumb encoding.
// RUN: %clang_cc1 -triple arm-unknown-linux -O0 -fsanitize-cfi-cross-dso \
// RUN:     -fsanitize=cfi-vcall \
// RUN:     -emit-llvm -o - %s | FileCheck --check-prefixes=CHECK,CHECK-ARM %s
//
// RUN: %clang_cc1 -triple thumb-unknown-linux -O0 -fsanitize-cfi-cross-dso \
// RUN:     -fsanitize=cfi-vcall \
// RUN:     -emit-llvm -o - %s | FileCheck --check-prefixes=CHECK,CHECK-THUMB %s
//
// REQUIRES: arm-registered-target

// CHECK-ARM: define weak_odr hidden void @__cfi_check_fail(i8*, i8*) #[[ARM:.*]] {
// CHECK-THUMB: define weak_odr hidden void @__cfi_check_fail(i8*, i8*) #[[THUMB:.*]] {
// CHECK: define weak void @__cfi_check(i64, i8*, i8*) #[[THUMB:.*]] {

// CHECK-ARM: attributes #[[ARM]] = {{.*}}"target-features"="{{.*}}-thumb-mode
// CHECK: attributes #[[THUMB]] = {{.*}}"target-features"="{{.*}}+thumb-mode
