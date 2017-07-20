// RUN: %clang_cc1 -std=c++1z -fsyntax-only -verify %s

struct X { };

template<typename T> T& lvalue();
template<typename T> T&& xvalue();
template<typename T> T prvalue();

// In a .* expression whose object expression is an rvalue, the
// program is ill-formed if the second operand is a pointer to member
// function with (non-const, C++17) ref-qualifier &.
// In a ->* expression or in a .* expression whose object
// expression is an lvalue, the program is ill-formed if the second
// operand is a pointer to member function with ref-qualifier &&.
void test(X *xp, int (X::*pmf)(int), int (X::*l_pmf)(int) &, 
          int (X::*r_pmf)(int) &&, int (X::*cl_pmf)(int) const &,
          int (X::*cr_pmf)(int) const &&) {
  // No ref-qualifier.
  (lvalue<X>().*pmf)(17);
  (xvalue<X>().*pmf)(17);
  (prvalue<X>().*pmf)(17);
  (xp->*pmf)(17);

  // Lvalue ref-qualifier.
  (lvalue<X>().*l_pmf)(17);
  (xvalue<X>().*l_pmf)(17); // expected-error-re{{pointer-to-member function type 'int (X::*)(int){{( __attribute__\(\(thiscall\)\))?}} &' can only be called on an lvalue}}
  (prvalue<X>().*l_pmf)(17); // expected-error-re{{pointer-to-member function type 'int (X::*)(int){{( __attribute__\(\(thiscall\)\))?}} &' can only be called on an lvalue}}
  (xp->*l_pmf)(17);

  // Rvalue ref-qualifier.
  (lvalue<X>().*r_pmf)(17); // expected-error-re{{pointer-to-member function type 'int (X::*)(int){{( __attribute__\(\(thiscall\)\))?}} &&' can only be called on an rvalue}}
  (xvalue<X>().*r_pmf)(17);
  (prvalue<X>().*r_pmf)(17);
  (xp->*r_pmf)(17);  // expected-error-re{{pointer-to-member function type 'int (X::*)(int){{( __attribute__\(\(thiscall\)\))?}} &&' can only be called on an rvalue}}

  // Lvalue const ref-qualifier.
  (lvalue<X>().*cl_pmf)(17);
  (xvalue<X>().*cl_pmf)(17);
  (prvalue<X>().*cl_pmf)(17);
  (xp->*cl_pmf)(17);

  // Rvalue const ref-qualifier.
  (lvalue<X>().*cr_pmf)(17); // expected-error-re{{pointer-to-member function type 'int (X::*)(int){{( __attribute__\(\(thiscall\)\))?}} const &&' can only be called on an rvalue}}
  (xvalue<X>().*cr_pmf)(17);
  (prvalue<X>().*cr_pmf)(17);
  (xp->*cr_pmf)(17);  // expected-error-re{{pointer-to-member function type 'int (X::*)(int){{( __attribute__\(\(thiscall\)\))?}} const &&' can only be called on an rvalue}}
}
