// RUN: %clang_cc1 -std=c++2a -verify %s
// expected-no-diagnostics

// This is a slightly modified version of the example from C++2a [class.mem]p7.
// It tests the resolution of parsing ambiguities.

int a;
struct S {
  unsigned x1 : 8 = 42;
  unsigned x2 : 8 {42};

  unsigned y1 : true ? 8 : a = 42;
  unsigned y2 : (true ? 8 : a) = 42;

  unsigned z1 : 1 || new int { 1 };
  unsigned z2 : (1 || new int) { 1 };
};

constexpr S s{};
static_assert(s.x1 == 42 && s.x2 == 42);
static_assert(s.y1 == 0  && s.y2 == 42);
static_assert(s.z1 == 0  && s.z2 == 1);
