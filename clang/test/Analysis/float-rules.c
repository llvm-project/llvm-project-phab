// RUN: %clang_analyze_cc1 -analyzer-checker=core.builtin,alpha.core.FPMath,debug.ExprInspection -verify %s
// REQUIRES: z3

double acos(double x);
double asin(double x);
double acosh(double x);
double atanh(double x);
double log(double x);
double log2(double x);
double log10(double x);
double log1p(double x);
double logb(double x);
double sqrt(double x);
double fmod(double x, double y);
double remainder(double x, double y);

double domain1() {
  double nan = __builtin_nan("");
  double z = 0.0;

  // -1 <= x <= 1
  z = acos(-5.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = acos(5.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = acos(0.0);  // no-warning
  z = acos(nan);  // no-warning
  z = asin(-5.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = asin(5.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = asin(0.0);  // no-warning
  z = asin(nan);  // no-warning

  // x >= 1
  z = acosh(-5.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = acosh(-1.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = acosh(0.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = acosh(1.0);  // no-warning
  z = acosh(nan);  // no-warning

  // -1 < x < 1
  z = atanh(-1.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = atanh(1.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = atanh(0.0);  // no-warning
  z = atanh(nan);  // no-warning

  // x >= 0
  z = log(-1.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = log(-0.5);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = log(0.0);  // no-warning
  z = log(nan);  // no-warning
  z = log2(-1.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = log2(-0.5);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = log2(0.0);  // no-warning
  z = log2(nan);  // no-warning
  z = log10(-1.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = log10(-0.5);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = log10(0.0);  // no-warning
  z = log10(nan);  // no-warning
  z = sqrt(-1.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = sqrt(-0.5);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = sqrt(0.0);  // no-warning
  z = sqrt(nan);  // no-warning

  // x >= -1
  z = log1p(-5.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = log1p(-1.0);  // no-warning
  z = log1p(0.0);  // no-warning
  z = log1p(nan);  // no-warning

  // x != 0
  z = logb(0.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = logb(-1.0);  // no-warning
  z = logb(1.0);  // no-warning
  z = logb(nan);  // no-warning

  return z;
}

double domain2(double x) {
  double nan = __builtin_nan("");
  double z = 0.0;

  // y != 0
  z = fmod(x, 0.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = fmod(x, 1.0);  // no-warning
  z = fmod(x, -1.0);  // no-warning
  z = fmod(x, nan);  // no-warning
  z = remainder(x, 0.0);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = remainder(x, 1.0);  // no-warning
  z = remainder(x, -1.0);  // no-warning
  z = remainder(x, nan);  // no-warning

  return z;
}

double domain3(double x) {
  double z = 0.0;

  if (__builtin_isnan(x) || x < 0)
    return z;

  z = acosh(x);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = asin(x);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = logb(x);  // expected-warning{{Argument value is out of valid domain for function call}}
  z = log1p(x);  // no-warning
  z = log(x);  // no-warning

  return z;
}
