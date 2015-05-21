// RUN: %clang_cc1 -std=c89 %s -fsyntax-only -verify -triple=i686-mingw32
// expected-no-diagnostics
// In C89, pretend these builtins don't exist.

int __builtin_isnan(double);
int __builtin_isinf(float);
int __builtin_isfinite(int);
int __builtin_isunordered(double, float);
int __builtin_islessgreater();
