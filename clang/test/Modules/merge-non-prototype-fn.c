// RUN: rm -rf %t
// RUN: %clang_cc1 -I%S/Inputs/merge-non-prototype-fn -fmodules -fimplicit-module-maps -x c -fmodules-cache-path=%t -fsyntax-only -verify %s

char *func1();
char *func2(const char*, int);

#include "header2.h"

void foo1(void) {
  func1("abc", 12);
  func2(1, 2, 3, 4); // expected-error {{too many arguments to function call, expected 2, have 4}}
                     // expected-note@header2.h:5 {{'func2' declared here}}
}
