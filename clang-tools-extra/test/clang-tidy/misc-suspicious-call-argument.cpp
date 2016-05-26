// RUN: %check_clang_tidy %s misc-suspicious-call-argument %t -- -- -std=c++11

// For equality test.
void set(int *buff, int value, int count) {
  while (count > 0) {
    *buff = value;
    ++buff;
    --count;
  }
}

// For equality test.
void foo(int *array, int count) {
  // Equality test.
  set(array, count, 10);
  // CHECK-MESSAGES: :[[@LINE-1]]:4: warning: count (value) is potentially swapped with literal (count). [misc-suspicious-call-argument]
}

// For equality test.
void f(int b, int aaa) {}

// For prefix/suffix test.
int uselessFoo(int valToRet, int uselessParam) { return valToRet; }

// For variadic function test.
double average(int count, int fixParam1, int fixParam2, ...) { return 0.0; }

void fooo(int asdasdasd, int bbb, int ccc = 1, int ddd = 0) {}

class TestClass {
public:
  void thisFunction(int integerParam, int thisIsPARAM) {}
};

int main() {
  int asdasdasd, qweqweqweq, cccccc, ddd = 0;

  fooo(asdasdasd, cccccc);
  // CHECK-MESSAGES: :[[@LINE-1]]:2: warning: cccccc (bbb) is potentially swapped with literal (ccc). [misc-suspicious-call-argument]

  const int MAX = 15;
  int *array = new int[MAX];
  int count = 5;

  // Equality test 1.
  foo(array, count);

  // Equality test 2.
  double avg = average(1, 2, count, 3, 4, 5, 6,
                       7); // "count" should be the first argument
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: literal (count) is potentially swapped with count (fixParam2). [misc-suspicious-call-argument]

  // Equality test 3.
  int aa, aaa = 0;
  f(aa, aaa); // should pass (0. arg is similar to 1. param, but 1. arg is equal to 1. param)

  // Equality test 4.
  int fixParam1 = 5;
  avg = average(count, 1, 2, 3, 4, fixParam1, 6,
                7); // should pass, we only check params and args before "..."

  // Levenshtein test 1.
  int cooundt = 4;
  set(array, cooundt, 10); // "cooundt" is similar to "count"
  // CHECK-MESSAGES: :[[@LINE-1]]:2: warning: cooundt (value) is potentially swapped with literal (count). [misc-suspicious-call-argument]

  // Levenshtein test 2.
  int _value = 3;
  set(array, 5, _value); // "_value" is similar to "value"
  // CHECK-MESSAGES: :[[@LINE-1]]:2: warning: literal (value) is potentially swapped with _value (count). [misc-suspicious-call-argument]

  // Prefix test 1.
  int val = 1;
  int tmp = uselessFoo(5, val); // "val" is a prefix of "valToRet"
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: literal (valToRet) is potentially swapped with val (uselessParam). [misc-suspicious-call-argument]

  // Prefix test 2.
  int VALtoretAwesome = 1;
  tmp = uselessFoo(
      5, VALtoretAwesome); // "valToRet" is a prefix of "VALtoretAwesome"
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: literal (valToRet) is potentially swapped with VALtoretAwesome (uselessParam). [misc-suspicious-call-argument]

  // Suffix test 1.
  int param = 1;
  int randomValue = 5;
  tmp = uselessFoo(param, randomValue); // "param" is a suffix of "uselessParam"
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: param (valToRet) is swapped with randomValue (uselessParam). [misc-suspicious-call-argument]

  // Suffix test 2.
  int reallyUselessParam = 1;
  tmp = uselessFoo(reallyUselessParam,
                   5); // "uselessParam" is a suffix of "reallyUselessParam"
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: reallyUselessParam (valToRet) is potentially swapped with literal (uselessParam). [misc-suspicious-call-argument]

  // Test lambda.
  auto testMethod = [&](int method, int randomParam) { return 0; };
  int method = 0;
  testMethod(method, 0); // Should pass.

  // Member function test
  TestClass test;
  int integ, thisIsAnArg = 0;
  test.thisFunction(integ, thisIsAnArg); // Should pass.

  return 0;
}