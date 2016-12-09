// RUN: clang %s -march=haswell -O3 -S -o - | FileCheck %s

#include <x86intrin.h>

// CHECK-LABEL: sqrtd
// CHECK:       vsqrtpd (%rdi), %xmm0
// CHECK-NEXT:  retq

__m128d sqrtd2(double* v) {
  return _mm_setr_pd(__builtin_sqrt(v[0]), __builtin_sqrt(v[1]));
}

// CHECK-LABEL: sqrtf4
// CHECK:       vsqrtps (%rdi), %xmm0
// CHECK-NEXT:  retq

__m128 sqrtf4(float* v) {
  return _mm_setr_ps(__builtin_sqrtf(v[0]), __builtin_sqrtf(v[1]), __builtin_sqrtf(v[2]), __builtin_sqrtf(v[3]));
}
