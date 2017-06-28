// RUN: %clang_cc1 -emit-llvm -o - -triple arm-none-linux-gnueabi %s | FileCheck %s
// RUN: %clang_cc1 -DWITH_AMBIGUOUS -verify %s
// RUN: %clang_cc1 -DWITH_AMBIGUOUS -fnative-half-type -fallow-half-arguments-and-returns -verify %s

typedef __fp16 half;
half x;

void half_to_float_or_double(float x);
void half_to_float_or_double(double x);

// CHECK:     call void @_Z23half_to_float_or_doubled
// CHECK-NOT: call void @_Z23half_to_float_or_doublef
void test1() {
  half_to_float_or_double(x);
}

extern void half_to_int_or_double(int x);
extern void half_to_int_or_double(double x);

// CHECK:     call void @_Z21half_to_int_or_doubled
// CHECK-NOT: call void @_Z21half_to_int_or_doublei
void test2() {
  half_to_int_or_double(x);
}

#ifdef WITH_AMBIGUOUS
extern void half_to_int_or_float(int x);    // expected-note {{candidate function}}
extern void half_to_int_or_float(float x);  // expected-note {{candidate function}}

void test3() {
  half_to_int_or_float(x);  // expected-error {{call to 'half_to_int_or_float' is ambiguous}}
}
#endif
