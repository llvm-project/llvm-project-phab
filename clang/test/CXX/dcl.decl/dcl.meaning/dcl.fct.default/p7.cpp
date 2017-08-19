// RUN: %clang_cc1 -fsyntax-only -verify %s
// expected-no-diagnostics

void h()
{
  int i;
  // Do not warn about local variables referenced in default arguments if it is
  // in an unevaluated expression.
  extern void h2(int x = sizeof(i));
}
