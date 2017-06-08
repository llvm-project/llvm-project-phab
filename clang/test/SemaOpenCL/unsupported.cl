// RUN: %clang_cc1 -verify %s

struct {
  int a : 1; // expected-error {{bit-fields are not supported in OpenCL}}
};

void no_vla(int n) {
  int a[n]; // expected-error {{variable length arrays are not supported in OpenCL}}
}

void no_logxor(int n) {
  int logxor = n ^^ n; // expected-error {{^^ is a reserved operator in OpenCL}}
}

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

kernel void no_long_double(global long double *n) {} // expected-error {{'long double' is a reserved type in OpenCL}}

kernel void no_long_long(global long long *n) {} // expected-error {{'long long' is a reserved type in OpenCL}}
kernel void no_unsigned_long_long(global unsigned long long *n) {} // expected-error {{'unsigned long long' is a reserved type in OpenCL}}
