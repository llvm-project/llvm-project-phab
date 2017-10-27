// RUN: clang-tidy %s -checks="-*,misc-suspicious-semicolon" -- -DERROR 2>&1 \
// RUN:   | FileCheck %s -check-prefix=CHECK-ERROR \
// RUN:       -implicit-check-not="{{warning|error}}:"
// RUN: not clang-tidy %s -checks="-*,misc-suspicious-semicolon,clang-diagnostic-unused-variable" \
// RUN:   -warnings-as-errors=clang-diagnostic-unused-variable -- -DWERROR 2>&1 \
// RUN:   | FileCheck %s -check-prefix=CHECK-WERROR \
// RUN:       -implicit-check-not="{{warning|error}}:"

// Note: This test verifies that, the checker does not emit any warning for
//       files that do not compile.

bool g();

void f() {
  if (g());
  // CHECK-WERROR: :[[@LINE-1]]:11: warning: potentially unintended semicolon [misc-suspicious-semicolon]
#if ERROR
  int a
  // CHECK-ERROR: :[[@LINE-1]]:8: error: expected ';' at end of declaration [clang-diagnostic-error]
#elif WERROR
  int a;
  // CHECK-WERROR: :[[@LINE-1]]:7: error: unused variable 'a' [clang-diagnostic-unused-variable,-warnings-as-errors]
#else
#error "One of ERROR or WERROR should be defined.
#endif
}
