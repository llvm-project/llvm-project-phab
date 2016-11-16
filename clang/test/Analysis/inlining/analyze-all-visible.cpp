// RUN: %clang_cc1 -analyze -analyzer-checker=debug.ExprInspection -analyzer-config skip-inlined-functions=false -analyzer-display-progress %s 2>&1 | FileCheck %s

namespace c_like {

// static functions are not analyzed again if they are inlined
static int static_func() {
  return 0;
}

// We analyze callee() again despite the fact it was inlined
// because it is externally visible
int callee(int arg) {
  if (arg == 1)
    return 1; // expected-warning{{REACHABLE}}
  return 0;   // expected-warning{{REACHABLE}}
}

void caller() {
  callee(1);
  static_func();
}

}

namespace private_methods {

class Test {
public:
  void test() {
    callee(1);
  }
private:
  // We don't analyze private methods if they were inlined
  int callee(int arg) {
    if (arg == 1)
      return 1; // expected-warning{{REACHABLE}}
    return 0;   // no-warning
  }
};

}

// CHECK: analyze-all-visible.cpp c_like::static_func()
// CHECK-NEXT: analyze-all-visible.cpp c_like::callee(int)
// CHECK-NEXT: analyze-all-visible.cpp c_like::caller()
// CHECK-NEXT: analyze-all-visible.cpp private_methods::Test::test()
// CHECK-NEXT: analyze-all-visible.cpp private_methods::Test::callee(int)
// CHECK-NEXT: analyze-all-visible.cpp private_methods::Test::test()
// CHECK-NEXT: analyze-all-visible.cpp c_like::caller()
// CHECK-NEXT: analyze-all-visible.cpp c_like::callee(int)
