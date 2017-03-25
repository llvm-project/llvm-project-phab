// RUN: %check_clang_tidy %s cppcoreguidelines-pro-type-member-init %t -- -config="{CheckOptions: [{key: "cppcoreguidelines-pro-type-member-init.LiteralInitializers", value: 1}]}" -- -std=c++11

struct T {
  int i;
};

struct S {
  bool b;
  // CHECK-FIXES: bool b = false;
  int i;
  // CHECK-FIXES: int i = 0;
  float f;
  // CHECK-FIXES: float f = 0.0f;
  double d;
  // CHECK-FIXES: double d = 0.0;
  long double l;
  // CHECK-FIXES: double l = 0.0l;
  int *ptr;
  // CHECK-FIXES: int *ptr = nullptr;
  T t;
  // CHECK-FIXES: T t{};
  S() {};
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: constructor does not initialize these fields:
};
