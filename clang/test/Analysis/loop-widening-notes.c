// RUN: %clang_analyze_cc1 -analyzer-checker=core -analyzer-max-loop 2 -analyzer-config widen-loops=true -analyzer-output=text -verify %s

int *p_a;
int bar();
int flag_a;
int test_for_bug_25609() {
  if (p_a == 0) // expected-note {{Assuming 'p_a' is equal to null}} expected-note {{Taking true branch}}
    bar();
  for (int i = 0; i < flag_a; ++i) {} // expected-note {{Loop condition is true.  Entering loop body}}  expected-note {{Reach the maximum loop limit. Assigning a value to 'p_a'}} expected-note {{Loop condition is false. Execution continues on line 10}}
  *p_a = 25609; // no-crash expected-note {{Dereference of null pointer (loaded from variable 'p_a')}} expected-warning {{Dereference of null pointer (loaded from variable 'p_a')}}
  return *p_a;
}

int flag_b;
int while_analyzer_output() {
  int num = 0;
  int *p_b = &num;
  while (flag_b > 0) // expected-note {{Loop condition is true.  Entering loop body}} expected-note {{Reach the maximum loop limit. Assigning a value to 'p_b'}} expected-note {{Loop condition is false. Execution continues on line 22}}
  {
    flag_b--;
  }
  if (p_b == 0) // expected-note {{Assuming 'p_b' is equal to null}} expected-note {{Taking true branch}}
    num = 10;
  return *p_b; // no-crash expected-note {{Dereference of null pointer (loaded from variable 'p_b')}} expected-warning {{Dereference of null pointer (loaded from variable 'p_b')}}
}

int flag_c;
int do_while_analyzer_output() {
  int num = 10;
  do // expected-note {{Loop condition is true. Execution continues on line 32}} expected-note {{Loop condition is false.  Exiting loop}}
  {
    flag_c--;
  } while (flag_c); //expected-note {{Reach the maximum loop limit. Assigning a value to 'num'}}
  int local = 0;
  if (num == 0)       // expected-note{{Assuming 'num' is equal to 0}} expected-note{{Taking true branch}}
    local = 10 / num; // no-crash expected-note {{Division by zero}} expected-warning {{Division by zero}}
  return local;
}

int flag_d;
int *p_d;
int nested_loop_output() {
  if (p_d == 0) // expected-note {{Assuming 'p_d' is equal to null}} expected-note {{Taking true branch}}
    bar();
  for (int i = 0; i < flag_d; ++i)   // expected-note {{Loop condition is true.  Entering loop body}} expected-note {{Loop condition is false. Execution continues on line 48}}
    for (int j = 0; j < flag_d; ++j) // expected-note {{Loop condition is true.  Entering loop body}} expected-note {{Reach the maximum loop limit. Assigning a value to 'p_d'}} expected-note {{Loop condition is false. Execution continues on line 45}}
      ++i, ++j;
  return *p_d; // no-crash expected-note {{Dereference of null pointer (loaded from variable 'p_d')}} expected-warning {{Dereference of null pointer (loaded from variable 'p_d'}}
}

int flag_e;
int test_for_loop() {
  int num = 10;
  int tmp;
  for (int i = 0, tmp = 0; ++tmp,    // expected-note {{Loop condition is true.  Entering loop body}} expected-note {{Reach the maximum loop limit. Assigning a value to 'num'}} expected-note {{Loop condition is false. Execution continues on line 59}}
                       i < flag_e; ++i) {
    flag_e--;
  }
  if (num == 0)                      // expected-note {{Assuming 'num' is equal to 0}} expected-note {{Taking true branch}}
    flag_e = 10;                     
  return flag_e / num;               // expected-warning {{Division by zero}} expected-note {{Division by zero}}
}
