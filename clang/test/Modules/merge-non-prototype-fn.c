// RUN: rm -rf %t
// RUN: %clang_cc1 -I%S/Inputs/merge-non-prototype-fn -fmodules -fimplicit-module-maps -x c -fmodules-cache-path=%t -fsyntax-only -verify %s
// expected-no-diagnostics

char *func1();

#include "header2.h"

void foo1(void) {
  func1("abc", 12);
}
