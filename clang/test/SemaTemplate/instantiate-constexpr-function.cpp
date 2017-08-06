// RUN: %clang_cc1 -std=c++14 -verify %s
// expected-no-diagnostics

template<typename T> constexpr void func_01(T) {
  func_01(0);
}


template<typename T> constexpr unsigned func_02a() {
  return sizeof(T);
}
template<typename T> constexpr T func_02b(T x) {
  return x + func_02a<int>();
}
constexpr long val_02 = func_02b(14L);


template<typename T> constexpr T func_03(T) {
  return T::xyz;
}
template<typename T> T func_04(T x) {
  return x;
}
template<> constexpr long func_04(long x) {
  return 66;
}
constexpr long var_04 = func_04(0L);
static_assert(var_04 == 66, "error");


template<typename T> struct C_05 {
  constexpr T func_05() { return T::xyz; }
};
C_05<int> var_05;
