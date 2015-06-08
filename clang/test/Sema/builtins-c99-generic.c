// RUN: %clang_cc1 -std=c99 %s -fsyntax-only -verify -triple=i686-mingw32

int __builtin_isnan(double);
int __builtin_isinf(float);

int __builtin_isunordered(double, float);
int __builtin_islessgreater(long double, double);
int __builtin_isgreater(float, float);

int __builtin_isfinite(int); // expected-error {{__builtin_isfinite expects either double, long double or float argument types, but has been given an argument of type int}}
int __builtin_isnormal(); // expected-error {{__builtin_isnormal is a builtin that takes 1 argument, it has been redeclared with 0 arguments}}
int __builtin_islessequal(double); // expected-error {{__builtin_islessequal is a builtin that takes 2 arguments, it has been redeclared with 1 argument}}
int __builtin_isless(); // expected-error {{__builtin_isless is a builtin that takes 2 arguments, it has been redeclared with 0 arguments}}
