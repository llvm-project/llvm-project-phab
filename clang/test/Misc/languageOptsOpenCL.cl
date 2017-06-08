// RUN: %clang_cc1 -x cl %s -verify -triple spir-unknown-unknown
// expected-no-diagnostics

// Test the forced language options for OpenCL are set correctly.

kernel void test() {
  int v0[(sizeof(int) == 4) - 1];
  int v1[(__alignof(int)== 4) - 1];
  int v2[(sizeof(long) == 8) - 1];
  int v3[(__alignof(long)== 8) - 1];
  int v4[(sizeof(float) == 4) - 1];
  int v5[(__alignof(float)== 4) - 1];
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
  int v6[(sizeof(double) == 8) - 1];
  int v7[(__alignof(double)== 8) - 1];
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
  int v8[(sizeof(half) == 2) - 1];
  int v9[(__alignof(half) == 2) - 1];
}
