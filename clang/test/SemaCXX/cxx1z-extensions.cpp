// RUN: %clang_cc1 -std=c++11 -Wno-c++1z-extensions -Wno-c++14-extensions -verify %s

// expected-no-diagnostics

// Fold Expressions //
static_assert(__has_extension(cxx_fold_expressions), "");
template <class ...Bools>
constexpr bool test_fold_expressions(Bools ...b) {
  return (b && ...);
}
static_assert(test_fold_expressions(true, true, true), "");

// If Constexpr //
static_assert(__has_extension(cxx_if_constexpr), "");
template <bool Pred>
constexpr bool test_if_constexpr() {
  if constexpr (Pred) {
    static_assert(Pred, "");
    return true;
  } else {
    static_assert(!Pred, "");
    return false;
  }
}
static_assert(test_if_constexpr<true>(), "");
static_assert(!test_if_constexpr<false>(), "");

// Inline Variables //
static_assert(__has_extension(cxx_inline_variables), "");
inline int test_inline_variables = 42;

// Structured Bindings //
static_assert(__has_extension(cxx_structured_bindings), "");
struct Tuple {
  int a, b, c;
};
constexpr bool test_bindings() {
  auto [a, b, c] = Tuple{42, 101, -1};
  return a == 42 && b == 101 && c == -1;
}
static_assert(test_bindings(), "");

// Variadic Using //
static_assert(__has_extension(cxx_variadic_using), "");
struct A { constexpr bool operator()() const { return true; } };
struct B { constexpr bool operator()(int) const { return false; } };
template <class ...Args> struct TestVariadicUsing : Args... {
  using Args::operator()...;
};
static_assert(TestVariadicUsing<A, B>{}(), "");
static_assert(!TestVariadicUsing<A, B>{}(42), "");
