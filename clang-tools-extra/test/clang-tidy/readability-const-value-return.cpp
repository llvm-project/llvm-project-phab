// RUN: %check_clang_tidy %s readability-const-value-return %t

struct X {
  int a;
};

// This should trigger the check.
const X f_returns_const_value();
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: function 'f_returns_const_value' has a const value return type; this can cause unnecessary copies [readability-const-value-return]

// These should not trigger the check.
X f_returns_nonconst_value();
const X* f_returns_pointer_to_const();
X* const f_returns_const_pointer();
const X& f_returns_reference_to_const();

typedef X* Xptr;
typedef X& Xref;
const Xptr f_returns_typedef_pointer();
const Xref f_returns_typedef_reference();

// A template parameter type may end up being a pointer or a reference, so we skip it too.
template <typename T>
const T f_returns_template_param();

template <typename T>
struct S {
  using Q = T;
  const Q f_returns_template_param_alias();
};

using R = X&;
const R f_returns_reference_alias();
