// RUN: %clang_cc1 -fsyntax-only -verify -std=c++98 %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

template <class T>
struct Base{};

template <class A, class B>
class Test {};

template <class T>
class Test<void, void> : Base<T> { // expected-error{{partial specialization of 'Test' does not use any of its template parameters}}
  Test() {}
};
