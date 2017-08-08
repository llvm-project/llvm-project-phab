// RUN: %clang_analyze_cc1 -analyzer-checker=core,debug.ExprInspection -verify %s

void clang_analyzer_eval(int);

void test1(int a) {
  if (a >= 10 && a <= 50) {
    int b;

    b = a + 2;
    clang_analyzer_eval(b >= 12 && b <= 52); // expected-warning{{TRUE}}

    b = a - 2;
    clang_analyzer_eval(b >= 8 && b <= 48); // expected-warning{{TRUE}}

    b = a / 2;
    clang_analyzer_eval(b >= 5 && b <= 25); // expected-warning{{TRUE}}
  }
}
