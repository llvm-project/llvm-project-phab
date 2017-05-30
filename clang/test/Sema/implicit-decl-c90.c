// RUN: %clang_cc1 %s -std=c90 -verify -fsyntax-only
int f() {
  return g() + 1;
}

int (*p)() = g; /* expected-error {{use of undeclared identifier 'g'}} */

float g(); /* not expecting conflicting types diagnostics here */
