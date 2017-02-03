// RUN: %clangxx -c %s -o %s.o
// RUN: %clangxx -o %t -nostartfiles -nostdlib -nodefaultlibs %crt1 %crti %crtbegin %s.o -lc -lm -lgcc %crtend %crtn
// RUN: %run %t 2>&1 | FileCheck %s

#include <stdio.h>

struct A {
  A() {}
  ~A() {
    // CHECK: ~A()
    printf("~A()");
  }
};

A a;

int main() {
  return 0;
}
