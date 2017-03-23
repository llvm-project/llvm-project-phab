// RUN: %clangxx -DCRT_SHARED -c %s -fPIC -o %tshared.o
// RUN: %clangxx -c %s -fPIC -o %t.o
// RUN: %clangxx -shared -o %t.so -nostartfiles -nostdlib -nodefaultlibs %crti %shared_crtbegin %tshared.o -lc -lm -lgcc_s %shared_crtend %crtn
// RUN: %clangxx -o %t -nostartfiles -nostdlib -nodefaultlibs %crt1 %crti %crtbegin %t.o -lc -lm %libgcc %t.so %crtend %crtn
// RUN: %run %t 2>&1 | FileCheck %s

#include <stdio.h>

// CHECK: 1
// CHECK-NEXT: ~A()

#ifdef CRT_SHARED
bool G;
void C() {
  printf("%d\n", G);
}

struct A {
  A() { G = true; }
  ~A() {
    printf("~A()\n");
  }
};

A a;
#else
void C();

int main() {
  C();
  return 0;
}
#endif
