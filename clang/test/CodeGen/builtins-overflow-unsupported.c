// RUN: not %clang_cc1 -triple "i686-unknown-unknown"   -emit-llvm -x c %s -o - 2>&1 | FileCheck %s --check-prefix=M32
// RUN: not %clang_cc1 -triple "x86_64-unknown-unknown" -emit-llvm -x c %s -o - 2>&1 | FileCheck %s --check-prefix=M64

signed long long try_smul_i65(signed long long a, unsigned long long b) {
  // M32: [[@LINE+1]]:10: error: cannot compile this __builtin_mul_overflow with mixed-sign operands yet
  return __builtin_mul_overflow(a, b, &b);
}

#if defined(__LP64__)
__int128_t try_smul_i29(__int128_t a, __uint128_t b) {
  // M64: [[@LINE+1]]:10: error: cannot compile this __builtin_mul_overflow with mixed-sign operands yet
  return __builtin_mul_overflow(a, b, &b);
}
#endif
