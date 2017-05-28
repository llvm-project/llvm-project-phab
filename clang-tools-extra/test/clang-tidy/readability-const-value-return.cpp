// RUN: %check_clang_tidy %s readability-const-value-return %t

namespace std {
template<class> class function;
template<class R, class... ArgTypes> class function<R(ArgTypes...)>;
}

struct X {
  int a;
};

// These should trigger the check.
const X f_returns_const_value();
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: function 'f_returns_const_value' has a const value return type; this can cause unnecessary copies [readability-const-value-return]
// CHECK-FIXES: {{^}}X f_returns_const_value();{{$}}

X const f_returns_const_value2();
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: function 'f_returns_const_value2' has a const value return type; this can cause unnecessary copies [readability-const-value-return]
// CHECK-FIXES: {{^}}X f_returns_const_value2();{{$}}

// When using trailing return type syntax, we cannot generate the fixit, because we cannot get the return type range.
auto f_returns_const_value3() -> const X;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'f_returns_const_value3' has a const value return type; this can cause unnecessary copies [readability-const-value-return]

auto f_returns_const_value4() -> X const;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'f_returns_const_value4' has a const value return type; this can cause unnecessary copies [readability-const-value-return]

// Make sure we get the right "const".
const std::function<int(const char *)> f_returns_const_fun();
// CHECK-MESSAGES: :[[@LINE-1]]:40: warning: function 'f_returns_const_fun' has a const value return type; this can cause unnecessary copies [readability-const-value-return]
// CHECK-FIXES: {{^}}std::function<int(const char *)> f_returns_const_fun();{{$}}

std::function<int(const char *)> const f_returns_const_fun2();
// CHECK-MESSAGES: :[[@LINE-1]]:40: warning: function 'f_returns_const_fun2' has a const value return type; this can cause unnecessary copies [readability-const-value-return]
// CHECK-FIXES: {{^}}std::function<int(const char *)> f_returns_const_fun2();{{$}}

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
