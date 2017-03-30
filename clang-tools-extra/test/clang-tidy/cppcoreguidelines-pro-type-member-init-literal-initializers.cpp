// RUN: %check_clang_tidy %s cppcoreguidelines-pro-type-member-init %t -- -config="{CheckOptions: [{key: "cppcoreguidelines-pro-type-member-init.LiteralInitializers", value: 1}]}" -- -std=c++11

struct T {
  int i;
};

struct S {
  bool b;
  // CHECK-FIXES: bool b = false;
  char c;
  // CHECK-FIXES: char c = 0;
  signed char sc;
  // CHECK-FIXES: signed char sc = 0;
  unsigned char uc;
  // CHECK-FIXES: unsigned char uc = 0u;
  int i;
  // CHECK-FIXES: int i = 0;
  unsigned u;
  // CHECK-FIXES: unsigned u = 0u;
  long l;
  // CHECK-FIXES: long l = 0l;
  unsigned long ul;
  // CHECK-FIXES: unsigned long ul = 0ul;
  long long ll;
  // CHECK-FIXES: long long ll = 0ll;
  unsigned long long ull;
  // CHECK-FIXES: unsigned long long ull = 0ull;
  float f;
  // CHECK-FIXES: float f = 0.0f;
  double d;
  // CHECK-FIXES: double d = 0.0;
  long double ld;
  // CHECK-FIXES: double ld = 0.0l;
  int *ptr;
  // CHECK-FIXES: int *ptr = nullptr;
  T t;
  // CHECK-FIXES: T t{};
  S() {};
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: constructor does not initialize these fields:
};
