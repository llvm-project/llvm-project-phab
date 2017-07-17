// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s
// RUN: %clang_cc1 -S -triple i686-pc-linux-gnu -std=c++11 %s -o - | FileCheck %s
// expected-no-diagnostics

template<typename T> void func_01(T *x);
template<typename T1>
struct C1 {
  template<typename T> friend void func_01(T *x) {}
};

C1<int> v1a;

void use_01(int *x) {
  func_01(x);
}
// CHECK: _Z7func_01IiEvPT_:


template<typename T1>
struct C2 {
  template<typename T> friend void func_02(T *x) {}
};

C2<int> v2a;
template<typename T> void func_02(T *x);

void use_02(int *x) {
  func_02(x);
}
// CHECK: _Z7func_02IiEvPT_:


template<typename T1>
struct C3a {
  template<typename T> friend void func_03(T *x) {}
};
template<typename T1>
struct C3b {
  template<typename T> friend void func_03(T *x) {}
};

template<typename T> void func_03(T *x) {}

void use_03(int *x) {
  func_03(x);
}
// CHECK: _Z7func_03IiEvPT_:


template<typename T> constexpr int func_04(const T x);
template<typename T1>
struct C4 {
  template<typename T> friend constexpr int func_04(const T x) { return sizeof(T); }
};

C4<int> v4;

void use_04(int *x) {
  static_assert(func_04(short(122)) == sizeof(short), "Invalid calculation");
  static_assert(func_04(122) == sizeof(int), "Invalid calculation");
  static_assert(func_04(122L) == sizeof(long), "Invalid calculation");
  static_assert(func_04(122LL) == sizeof(long long), "Invalid calculation");
}
