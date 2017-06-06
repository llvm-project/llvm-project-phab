// RUN: %clang_cc1 %s -std=c90 -verify -fsyntax-only
void t0(int x) {
  int (*p)();
  if(x > 0)
    x = g() + 1;
  p = g;
  if(x < 0) {
    extern void u(int (*)[h()]);
    int (*q)() = h;
  }
  p = h; /* expected-error {{use of undeclared identifier 'h'}} */
}

void t1(int x) {
  int (*p)();
  switch (x) {
    g();
  case 0:
    x = h() + 1;
    break;
  case 1:
    p = g;
    p = h;
    break;
  }
  p = g; /* expected-error {{use of undeclared identifier 'g'}} */
  p = h; /* expected-error {{use of undeclared identifier 'h'}} */
}

int (*p)() = g; /* expected-error {{use of undeclared identifier 'g'}} */
int (*q)() = h; /* expected-error {{use of undeclared identifier 'h'}} */

float g(); /* not expecting conflicting types diagnostics here */
