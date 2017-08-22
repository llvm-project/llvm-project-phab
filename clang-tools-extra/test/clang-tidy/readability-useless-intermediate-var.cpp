// RUN: %check_clang_tidy %s readability-useless-intermediate-var %t

bool f() {
  auto test = 1; // Test
  // CHECK-FIXES: {{^}}  // Test{{$}}
  return (test == 1);
  // CHECK-FIXES: {{^}}  return (1 == 1);{{$}}
  // CHECK-MESSAGES: :[[@LINE-4]]:8: warning: intermediate variable 'test' is useless [readability-useless-intermediate-var]
  // CHECK-MESSAGES: :[[@LINE-3]]:11: note: because it is only used when returning the result of this comparison
  // CHECK-MESSAGES: :[[@LINE-6]]:8: note: consider removing the variable declaration
  // CHECK-MESSAGES: :[[@LINE-5]]:11: note: and directly using the variable initialization expression here
}

bool f2() {
  auto test1 = 1; // Test1
  // CHECK-FIXES: {{^}}  // Test1{{$}}
  auto test2 = 2; // Test2
  // CHECK-FIXES: {{^}}  // Test2{{$}}
  return (test1 == test2);
  // CHECK-FIXES: {{^}}  return (1 == 2);{{$}}
  // CHECK-MESSAGES: :[[@LINE-6]]:8: warning: intermediate variable 'test1' is useless [readability-useless-intermediate-var]
  // CHECK-MESSAGES: :[[@LINE-5]]:8: warning: and so is 'test2' [readability-useless-intermediate-var]
  // CHECK-MESSAGES: :[[@LINE-4]]:11: note: because they are only used when returning the result of this comparison
  // CHECK-MESSAGES: :[[@LINE-9]]:8: note: consider removing both this variable declaration
  // CHECK-MESSAGES: :[[@LINE-8]]:8: note: and this one
  // CHECK-MESSAGES: :[[@LINE-7]]:11: note: and directly using the variable initialization expressions here
}

bool f3() {
  auto test1 = 1; // Test1
  // CHECK-FIXES: {{^}}  // Test1{{$}}
  auto test2 = 2; // Test2
  return (test1 == 2);
  // CHECK-FIXES: {{^}}  return (1 == 2);{{$}}
  // CHECK-MESSAGES: :[[@LINE-5]]:8: warning: intermediate variable 'test1' is useless [readability-useless-intermediate-var]
  // CHECK-MESSAGES: :[[@LINE-3]]:11: note: because it is only used when returning the result of this comparison
  // CHECK-MESSAGES: :[[@LINE-7]]:8: note: consider removing the variable declaration
  // CHECK-MESSAGES: :[[@LINE-5]]:11: note: and directly using the variable initialization expression here
}

bool f4() {
  auto test1 = 1; // Test1
  auto test2 = 2; // Test2
  // CHECK-FIXES: {{^}}  // Test2{{$}}
  return (test2 == 3);
  // CHECK-FIXES: {{^}}  return (2 == 3);{{$}}
  // CHECK-MESSAGES: :[[@LINE-4]]:8: warning: intermediate variable 'test2' is useless [readability-useless-intermediate-var]
  // CHECK-MESSAGES: :[[@LINE-3]]:11: note: because it is only used when returning the result of this comparison
  // CHECK-MESSAGES: :[[@LINE-6]]:8: note: consider removing the variable declaration
  // CHECK-MESSAGES: :[[@LINE-5]]:11: note: and directly using the variable initialization expression here
}

bool f5() {
  auto test1 = 1; // Test1
  // CHECK-FIXES: {{^}}  // Test1{{$}}
  auto test2 = 2; // Test2
  return (2 == test1);
  // CHECK-FIXES: {{^}}  return (1 == 2);{{$}}
  // CHECK-MESSAGES: :[[@LINE-5]]:8: warning: intermediate variable 'test1' is useless [readability-useless-intermediate-var]
  // CHECK-MESSAGES: :[[@LINE-3]]:16: note: because it is only used when returning the result of this comparison
  // CHECK-MESSAGES: :[[@LINE-7]]:8: note: consider removing the variable declaration
  // CHECK-MESSAGES: :[[@LINE-5]]:11: note: and directly using the variable initialization expression here
}

bool f6() {
  auto test1 = 1; // Test1
  auto test2 = 2; // Test2
  // CHECK-FIXES: {{^}}  // Test2{{$}}
  return (3 == test2);
  // CHECK-FIXES: {{^}}  return (2 == 3);{{$}}
  // CHECK-MESSAGES: :[[@LINE-4]]:8: warning: intermediate variable 'test2' is useless [readability-useless-intermediate-var]
  // CHECK-MESSAGES: :[[@LINE-3]]:16: note: because it is only used when returning the result of this comparison
  // CHECK-MESSAGES: :[[@LINE-6]]:8: note: consider removing the variable declaration
  // CHECK-MESSAGES: :[[@LINE-5]]:11: note: and directly using the variable initialization expression here
}

bool f_operator_inversion() {
  auto test1 = 1; // Test1
  // CHECK-FIXES: {{^}}  // Test1{{$}}
  return (2 > test1);
  // CHECK-FIXES: {{^}}  return (1 < 2);{{$}}
  // CHECK-MESSAGES: :[[@LINE-4]]:8: warning: intermediate variable 'test1' is useless [readability-useless-intermediate-var]
  // CHECK-MESSAGES: :[[@LINE-3]]:15: note: because it is only used when returning the result of this comparison
  // CHECK-MESSAGES: :[[@LINE-6]]:8: note: consider removing the variable declaration
  // CHECK-MESSAGES: :[[@LINE-5]]:11: note: and directly using the variable initialization expression here
}

bool f_init2_contains_var1() {
  auto test1 = 1; // Test1
  auto test2 = test1; // Test2
  // CHECK-FIXES: {{^}}  // Test2{{$}}
  return (2 == test2);
  // CHECK-FIXES: {{^}}  return (test1 == 2);{{$}}
  // CHECK-MESSAGES: :[[@LINE-4]]:8: warning: intermediate variable 'test2' is useless [readability-useless-intermediate-var]
  // CHECK-MESSAGES: :[[@LINE-3]]:16: note: because it is only used when returning the result of this comparison
  // CHECK-MESSAGES: :[[@LINE-6]]:8: note: consider removing the variable declaration
  // CHECK-MESSAGES: :[[@LINE-5]]:11: note: and directly using the variable initialization expression here
}

bool f_double_use() {
  auto test = 1;
  return (test == (test + 1));
}

bool f_double_use2() {
  auto test1 = 1;
  auto test2 = 2;
  return (test1 == (test1 + 1));
}

bool f_double_use3() {
  auto test1 = 1;
  auto test2 = 2;
  return (test2 == (test2 + 1));
}

bool f_double_use4() {
  auto test1 = 1;
  auto test2 = 2;
  return ((test1 + 1) == test1);
}

bool f_double_use5() {
  auto test1 = 1;
  auto test2 = 2;
  return ((test2 + 1) == test2);
}

bool f_intermediate_statement() {
  auto test = 1;
  test = 2;
  return (test == 1);
}

bool f_long_expression() {
  auto test = "this is a very very very very very very very very very long expression to test max line length detection";
  return (test == "");
}
