// RUN: %clang_analyze_cc1 -analyzer-checker=debug.ExprInspection -verify %s

void clang_analyzer_eval(int);

void equal(int m, int n) {
  if (m != n)
    return;
  clang_analyzer_eval(n == m); // expected-warning{{TRUE}}
}

void non_equal(int m, int n) {
  if (m == n)
    return;
  clang_analyzer_eval(n != m); // expected-warning{{TRUE}}
}

void less_or_equal(int m, int n) {
  if (m < n)
    return;
  clang_analyzer_eval(n <= m); // expected-warning{{TRUE}}
}

void less(int m, int n) {
  if (m <= n)
    return;
  clang_analyzer_eval(n < m); // expected-warning{{TRUE}}
}

void greater_or_equal(int m, int n) {
  if (m > n)
    return;
  clang_analyzer_eval(n >= m); // expected-warning{{TRUE}}
}

void greater(int m, int n) {
  if (m >= n)
    return;
  clang_analyzer_eval(n > m); // expected-warning{{TRUE}}
}
