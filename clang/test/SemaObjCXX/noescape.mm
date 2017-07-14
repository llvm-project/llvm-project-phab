// RUN: %clang_cc1 -fsyntax-only -verify -fblocks -std=c++11 %s

typedef void (^BlockTy)();

struct S {
  int i;
  void m();
};

void noescapeFunc0(id, __attribute__((noescape)) BlockTy);
void noescapeFunc1(id, [[clang::noescape]] BlockTy);
void noescapeFunc2(__attribute__((noescape)) int *);
void noescapeFunc3(__attribute__((noescape)) id);
void noescapeFunc4(__attribute__((noescape)) int &);

void invalidFunc0(int __attribute__((noescape))); // expected-warning {{'noescape' attribute only applies to pointer arguments}}
void invalidFunc1(int __attribute__((noescape(0)))); // expected-error {{'noescape' attribute takes no arguments}}
void invalidFunc2(int0 *__attribute__((noescape))); // expected-error {{use of undeclared identifier 'int0'; did you mean 'int'?}}
void invalidFunc3(__attribute__((noescape)) int (S::*Ty)); // expected-warning {{'noescape' attribute only applies to pointer arguments}}
void invalidFunc4(__attribute__((noescape)) void (S::*Ty)()); // expected-warning {{'noescape' attribute only applies to pointer arguments}}
int __attribute__((noescape)) g; // expected-warning {{'noescape' attribute only applies to parameters}}
