// RUN: %clang_cc1 -std=c++2a %s -verify

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
  if (auto [ok, d] = f())
    ;
  if (auto [ok, d] = g()) // expected-error {{value of type 'Na' is not contextually convertible to 'bool'}}
    ;
}
} // namespace CondInIf

namespace CondInWhile {
void h() {
  while (auto [ok, d] = f())
    ;
  while (auto [ok, d] = g()) // expected-error {{value of type 'Na' is not contextually convertible to 'bool'}}
    ;
}
} // namespace CondInWhile

namespace CondInFor {
void h() {
  for (; auto [ok, d] = f();)
    ;
  for (; auto [ok, d] = g();) // expected-error {{value of type 'Na' is not contextually convertible to 'bool'}}
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
  switch (auto [ok, d] = x)
    ;
  switch (auto [ok, d] = g()) // expected-error {{statement requires expression of integer type ('Na' invalid)}}
    ;
}
} // namespace CondInSwitch
