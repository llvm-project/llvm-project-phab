// RUN: %clang_cc1 -std=c++1z %s -verify

struct Na {
  bool flag;
  float data;
};

struct Rst {
  bool flag;
  float data;
  explicit operator bool() const {
    return flag;
  }
};

Rst f();
Na g();

namespace CondInIf {
void h() {
  if (auto [ok, d] = f()) // expected-warning {{decomposed condition is a Clang extension}}
    ;
  if (auto [ok, d] = g()) // expected-warning {{decomposed condition is a Clang extension}} expected-error {{value of type 'Na' is not contextually convertible to 'bool'}}
    ;
}
} // namespace CondInIf

namespace CondInWhile {
void h() {
  while (auto [ok, d] = f()) // expected-warning {{decomposed condition is a Clang extension}}
    ;
  while (auto [ok, d] = g()) // expected-warning {{decomposed condition is a Clang extension}} expected-error {{value of type 'Na' is not contextually convertible to 'bool'}}
    ;
}
} // namespace CondInWhile

namespace CondInFor {
void h() {
  for (; auto [ok, d] = f();) // expected-warning {{decomposed condition is a Clang extension}}
    ;
  for (; auto [ok, d] = g();) // expected-warning {{decomposed condition is a Clang extension}} expected-error {{value of type 'Na' is not contextually convertible to 'bool'}}
    ;
}
} // namespace CondInFor

struct IntegerLike {
  bool flag;
  float data;
  operator int() const {
    return int(data);
  }
};

namespace CondInSwitch {
void h(IntegerLike x) {
  switch (auto [ok, d] = x) // expected-warning {{decomposed condition is a Clang extension}}
    ;
  switch (auto [ok, d] = g()) // expected-warning {{decomposed condition is a Clang extension}} expected-error {{statement requires expression of integer type ('Na' invalid)}}
    ;
}
} // namespace CondInSwitch
