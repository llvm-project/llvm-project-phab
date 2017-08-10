// RUN: %clang_cc1 -fsyntax-only -verify %s

void escapefunc(int *);
void noescapefunc(__attribute__((noescape)) int *); // expected-note {{previous declaration is here}}
void (*fnptr0)(int *);
void (*fnptr1)(__attribute__((noescape)) int *);
void noescapefunc(int *); // expected-error {{conflicting types for 'noescapefunc'}}

void test0() {
  fnptr0 = &escapefunc;
  fnptr0 = &noescapefunc;
  fnptr1 = &escapefunc; // expected-warning {{incompatible function pointer types assigning to 'void (*)(__attribute__((noescape)) int *)' from 'void (*)(int *)'}}
  fnptr1 = &noescapefunc;
}
