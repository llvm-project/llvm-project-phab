// RUN: %clang_tbaasan -O0 %s -o %t && %run %t >%t.out 2>&1
// RUN: FileCheck %s < %t.out

// XFAIL: *
// Clang's TBAA representation currently get's this wrong (writes to different
// members of a union type are allowed to alias (i.e. change the type)).

#include <stdio.h>

// CHECK-NOT: ERROR: TBAASanitizer: type-aliasing-violation

int main() {
  union {
    int i;
    short s;
  } u;

  u.i = 42;
  u.s = 1;

  printf("%dn", u.i);
}
 
