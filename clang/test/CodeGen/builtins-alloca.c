// RUN: %clang_cc1 -triple x86_64-unknown-unknown -emit-llvm -o - %s | \
// RUN:   FileCheck %s -check-prefix=CHECK_X86_64
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -emit-llvm -o - %s -target-feature +avx | \
// RUN:   FileCheck %s -check-prefix=CHECK_X86_64_AVX
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -emit-llvm -o - %s -target-feature +avx512f | \
// RUN:   FileCheck %s -check-prefix=CHECK_X86_64_AVX512
// RUN: %clang_cc1 -triple i386-unknown-linux-android -emit-llvm -o - %s | \
// RUN:   FileCheck %s -check-prefix=CHECK_ANDROID_X86_32
// RUN: %clang_cc1 -triple i386-unknown-linux-android -emit-llvm -o - %s -target-feature +sse | \
// RUN:   FileCheck %s -check-prefix=CHECK_ANDROID_X86_32_SSE
// RUN: %clang_cc1 -triple i386-unknown-linux-android -emit-llvm -o - %s -target-feature +avx | \
// RUN:   FileCheck %s -check-prefix=CHECK_ANDROID_X86_32_AVX
// RUN: %clang_cc1 -triple i386-unknown-linux-android -emit-llvm -o - %s -target-feature +avx512f | \
// RUN:   FileCheck %s -check-prefix=CHECK_ANDROID_X86_32_AVX512
// RUN: %clang_cc1 -triple x86_64-unknown-linux-android -emit-llvm -o - %s | \
// RUN:   FileCheck %s -check-prefix=CHECK_ANDROID_X86_64
// RUN: %clang_cc1 -triple i386-unknown-unknown -emit-llvm -o - %s | \
// RUN:   FileCheck %s -check-prefix=CHECK_I386
// RUN: %clang_cc1 -triple i386-unknown-unknown -emit-llvm -o - %s -target-feature +avx | \
// RUN:   FileCheck %s -check-prefix=CHECK_I386_AVX
// RUN: %clang_cc1 -triple i386-unknown-unknown -emit-llvm -o - %s -target-feature +avx512f | \
// RUN:   FileCheck %s -check-prefix=CHECK_I386_AVX512
// RUN: %clang_cc1 -triple i386-apple-darwin -emit-llvm -o - %s |	\
// RUN:   FileCheck %s -check-prefix=CHECK_I386_APPLE
// RUN: %clang_cc1 -triple x86_64-apple-darwin -emit-llvm -o - %s | \
// RUN:   FileCheck %s -check-prefix=CHECK_X86_64_APPLE
// RUN: %clang_cc1 -triple i386-pc-win32 -emit-llvm -o - %s | \
// RUN:   FileCheck %s -check-prefix=CHECK_I386_WIN
// RUN: %clang_cc1 -triple x86_64-unknown-windows -emit-llvm -o - %s | \
// RUN:   FileCheck %s -check-prefix=CHECK_X86_64_WIN
// RUN: %clang_cc1 -triple i386-unknown-cygwin -emit-llvm -o - %s | \
// RUN:   FileCheck %s -check-prefix=CHECK_I386_CYGWIN
// RUN: %clang_cc1 -triple x86_64-unknown-cygwin -emit-llvm -o - %s | \
// RUN:   FileCheck %s -check-prefix=CHECK_X86_64_CYGWIN



char *c1; 
// CHECK_X86_64: %1 = alloca i8, i64 1, align 16
// CHECK_X86_64_AVX: %1 = alloca i8, i64 1, align 32
// CHECK_X86_64_AVX512: %1 = alloca i8, i64 1, align 64
// CHECK_ANDROID_X86_32: %1 = alloca i8, align 16
// CHECK_ANDROID_X86_32_SSE: %1 = alloca i8, align 16
// CHECK_ANDROID_X86_32_AVX: %1 = alloca i8, align 32
// CHECK_ANDROID_X86_32_AVX512: %1 = alloca i8, align 64
// CHECK_ANDROID_X86_64: alloca i8, i64 1, align 16
// CHECK_I386: %1 = alloca i8, align 16
// CHECK_I386_AVX: %1 = alloca i8, align 32
// CHECK_I386_AVX512: %1 = alloca i8, align 64
// CHECK_I386_APPLE: %1 = alloca i8, align 16
// CHECK_X86_64_APPLE: %1 = alloca i8, i64 1, align 16
// CHECK_I386_WIN: %1 = alloca i8, align 16
// CHECK_X86_64_WIN: %1 = alloca i8, i64 1, align 16
// CHECK_I386_CYGWIN: %1 = alloca i8, align 16
// CHECK_X86_64_CYGWIN: %1 = alloca i8, i64 1, align 16
void f1() {
    c1 = __builtin_alloca(1);
}
