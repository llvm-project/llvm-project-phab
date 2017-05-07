// RUN: %clang_cc1 -std=c++03  -Wno-c++11-extensions -Wno-c++14-extensions \
// RUN:                        -Wno-c++1z-extensions -verify %s
// RUN: %clang_cc1 -std=c++11 -Wno-c++14-extensions -Wno-c++1z-extensions  -verify %s

// expected-no-diagnostics

// Fold Expressions //
_Static_assert(__has_extension(cxx_fold_expressions), "");
template <class ...Bools>
bool test_fold_expressions(Bools ...b) {
  return (b && ...);
}
static bool test_fold_expressions_result = test_fold_expressions(true, true, true);

#if __cplusplus >= 201103L
// If Constexpr //
_Static_assert(__has_extension(cxx_if_constexpr), "");
template <bool Pred>
constexpr bool test_if_constexpr() {
  if constexpr (Pred) {
    _Static_assert(Pred, "");
    return true;
  } else {
    _Static_assert(!Pred, "");
    return false;
  }
}
_Static_assert(test_if_constexpr<true>(), "");
_Static_assert(!test_if_constexpr<false>(), "");
#endif

// Inline Variables //
_Static_assert(__has_extension(cxx_inline_variables), "");
inline int test_inline_variables = 42;

// Structured Bindings //
_Static_assert(__has_extension(cxx_structured_bindings), "");
struct Tuple {
  int a, b, c;
};
bool test_bindings() {
  Tuple t = {42, 101, -1};
  auto [a, b, c] = t;
  return a == 42 && b == 101 && c == -1;
}

#if __cplusplus >= 201103L
// Variadic Using //
_Static_assert(__has_extension(cxx_variadic_using), "");
struct A { constexpr bool operator()() const { return true; } };
struct B { constexpr bool operator()(int) const { return false; } };
template <class ...Args> struct TestVariadicUsing : Args... {
  using Args::operator()...;
};
_Static_assert(TestVariadicUsing<A, B>{}(), "");
_Static_assert(!TestVariadicUsing<A, B>{}(42), "");
#endif
