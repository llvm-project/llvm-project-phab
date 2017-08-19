// RUN: %clang_cc1 -std=c++2a -verify %s

template<typename, typename>
constexpr bool is_same = false;

template<typename T>
constexpr bool is_same<T, T> = true;

template<typename T>
struct DummyTemplate { };

void func() {
  auto L0 = []<typename T>(T arg) {
    static_assert(is_same<T, int>);
  };
  L0(0);

  auto L1 = []<int I> {
    static_assert(I == 5);
  };
  L1.operator()<5>();

  auto L2 = []<template<typename> class T, class U>(T<U> &&arg) {
    static_assert(is_same<T<U>, DummyTemplate<float>>);
  };
  L2(DummyTemplate<float>());
}

template<typename T> // expected-note {{declared here}}
struct ShadowMe {
  void member_func() {
    auto L = []<typename T> { }; // expected-error {{'T' shadows template parameter}}
  }
};
