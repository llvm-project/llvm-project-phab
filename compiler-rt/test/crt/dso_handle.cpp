// RUN: %clangxx -DSHARED -c %s -fPIC -o %tshared.o
// RUN: %clangxx -c %s -fPIC -o %t.o
// RUN: %clangxx -shared -o %t.so -nostartfiles -nostdlib -nodefaultlibs %crti %shared_crtbegin %tshared.o -lc -lm -lgcc_s %shared_crtend %crtn
// RUN: %clangxx -o %t -nostartfiles -nostdlib -nodefaultlibs %crt1 %crti %crtbegin %t.o -lc -lm -lgcc %t.so %crtend %crtn
// RUN: %run %t 2>&1 | FileCheck %s

#include <stdio.h>

#ifdef SHARED
bool G;
void C() {
  // CHECK: 1
  printf("%d\n", G);
}

struct A {
  A() { G = true; }
  ~A() {
    // CHECK: ~A()
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
