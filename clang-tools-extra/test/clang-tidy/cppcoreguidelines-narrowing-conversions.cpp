// RUN: %check_clang_tidy %s cppcoreguidelines-narrowing-conversions %t

void not_ok(double d) {
  int i = 0;
  i += 0.5;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: narrowing conversion from 'double' to 'int' [cppcoreguidelines-narrowing-conversions]
  i += 0.5f;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: narrowing conversion from 'float' to 'int' [cppcoreguidelines-narrowing-conversions]
  i += d;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: narrowing conversion from 'double' to 'int' [cppcoreguidelines-narrowing-conversions]
  // We warn on the following even though it's not dangerous because there is no
  // reason to use a double literal here.
  // TODO(courbet): Provide an automatic fix.
  i += 2.0;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: narrowing conversion from 'double' to 'int' [cppcoreguidelines-narrowing-conversions]
  i += 2.0f;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: narrowing conversion from 'float' to 'int' [cppcoreguidelines-narrowing-conversions]
}

void ok(double d) {
  int i = 0;
  i += 1;
  i += static_cast<int>(3.0);
  i += static_cast<int>(d);
}
