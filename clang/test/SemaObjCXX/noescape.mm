// RUN: %clang_cc1 -fsyntax-only -verify -fblocks -std=c++11 %s

typedef void (^BlockTy)();

void noescapeFunc0(id, __attribute__((noescape)) BlockTy);
void noescapeFunc1(id, [[clang::noescape]] BlockTy);
void invalidFunc(int __attribute__((noescape))); // expected-warning {{'noescape' attribute ignored on parameter of non-pointer type}}
int __attribute__((noescape)) g; // expected-warning {{'noescape' attribute only applies to parameters}}
