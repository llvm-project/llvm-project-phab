// RUN: %clang_cc1 -fsyntax-only -verify %s

template<typename T> void take(T);

void func(float Real, _Complex float Complex) {
  Real += Complex; // expected-error {{assigning to 'float' from incompatible type '_Complex float'}}
  Real += (float)Complex;

  Real = Complex; // expected-error {{implicit conversion discards imaginary component: '_Complex float' to 'float'}}
  Real = (float)Complex;

  take<float>(Complex); // expected-error
  take<double>(1.0i); // expected-error {{implicit conversion discards imaginary component: '_Complex double' to 'double'}}
  take<_Complex float>(Complex);

  // Conversion to bool doesn't actually discard the imaginary part.
  take<bool>(Complex);
}
