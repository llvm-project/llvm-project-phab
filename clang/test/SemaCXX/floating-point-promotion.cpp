// RUN: %clang_cc1 -fsyntax-only -fallow-half-arguments-and-returns -verify %s
// RUN: %clang_cc1 -fsyntax-only -fallow-half-arguments-and-returns -fnative-half-type -verify %s

#include <stdint.h>

typedef __fp16 half;

// expected-note@floating-point-promotion.cpp:* 1+ {{candidate function}}

extern void half_to_int_or_float(int x);
extern void half_to_int_or_float(float x);
void test1(half h) {
  half_to_int_or_float(h);  // expected-error {{call to 'half_to_int_or_float' is ambiguous}}
}

extern void half_to_int_or_double(int x);
extern void half_to_int_or_double(double x);
void test2(half h) {
  half_to_int_or_double(h);
}

extern void half_to_int_or_long_double(int x);
extern void half_to_int_or_long_double(long double x);
void test3(half h) {
  half_to_int_or_long_double(h);  // expected-error {{call to 'half_to_int_or_long_double' is ambiguous}}
}

extern void half_to_float_or_double(float x);
extern void half_to_float_or_double(double x);
void test4(half h) {
  half_to_float_or_double(h);
}

extern void half_to_float_or_long_double(float x);
extern void half_to_float_or_long_double(long double x);
void test5(half h) {
  half_to_float_or_long_double(h);  // expected-error {{call to 'half_to_float_or_long_double' is ambiguous}}
}

extern void float_to_int_or_half(int x);
extern void float_to_int_or_half(half x);
void test6(float f) {
  float_to_int_or_half(f);  // expected-error {{call to 'float_to_int_or_half' is ambiguous}}
}

extern void float_to_int_or_double(int x);
extern void float_to_int_or_double(double x);
void test7(float f) {
  float_to_int_or_double(f);
}

extern void float_to_int_or_long_double(int x);
extern void float_to_int_or_long_double(long double x);
void test8(float f) {
  float_to_int_or_long_double(f);  // expected-error {{call to 'float_to_int_or_long_double' is ambiguous}}
}

extern void float_to_half_or_double(half x);
extern void float_to_half_or_double(double x);
void test9(float f) {
  float_to_half_or_double(f);
}

extern void float_to_half_or_long_double(half x);
extern void float_to_half_or_long_double(long double x);
void test10(float f) {
  float_to_half_or_long_double(f);  // expected-error {{call to 'float_to_half_or_long_double' is ambiguous}}
}

extern void double_to_int_or_half(int x);
extern void double_to_int_or_half(half x);
void test11(double d) {
  double_to_int_or_half(d);  // expected-error {{call to 'double_to_int_or_half' is ambiguous}}
}

extern void double_to_int_or_float(int x);
extern void double_to_int_or_float(float x);
void test12(double d) {
  double_to_int_or_float(d);  // expected-error {{call to 'double_to_int_or_float' is ambiguous}}
}

extern void double_to_int_or_long_double(int x);
extern void double_to_int_or_long_double(long double x);
void test13(double d) {
  double_to_int_or_long_double(d);  // expected-error {{call to 'double_to_int_or_long_double' is ambiguous}}
}

extern void double_to_half_or_float(half x);
extern void double_to_half_or_float(float x);
void test14(double d) {
  double_to_half_or_float(d);  // expected-error {{call to 'double_to_half_or_float' is ambiguous}}
}

extern void double_to_half_or_long_double(half x);
extern void double_to_half_or_long_double(long double x);
void test15(double d) {
  double_to_half_or_long_double(d);  // expected-error {{call to 'double_to_half_or_long_double' is ambiguous}}
}

extern void long_double_to_half_or_float(half x);
extern void long_double_to_half_or_float(float x);
void test16(long double ld) {
  double_to_half_or_float(ld);  // expected-error {{call to 'double_to_half_or_float' is ambiguous}}
}

extern void long_double_to_float_or_double(float x);
extern void long_double_to_float_or_double(double x);
void test17(long double ld) {
  long_double_to_float_or_double(ld);  // expected-error {{call to 'long_double_to_float_or_double' is ambiguous}}
}
