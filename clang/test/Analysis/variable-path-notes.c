// RUN: %clang_analyze_cc1 %s -analyzer-checker=core -analyzer-output=text -verify

void f(int);

void testNonLocConcreteInt() {
  int v[3], i;
  v[0] = 0; v[2] = 2;
  for (i = 0; i < 3; ++i) {
    // expected-note@-1 {{Loop condition is true.  Entering loop body}}
    // expected-note@-2 {{Loop condition is true.  Entering loop body}}
    f(v[i]); // expected-warning{{1st function call argument is an uninitialized value}}
    // expected-note@-1 {{1st function call argument is an uninitialized value}}
    // expected-note@-2 {{Assuming 'i' == 1}}
  }
}

void init(int*);
void assert(int);

void testSingleValueRangeSymbol(unsigned size) {
  int val;
  // expected-note@-1 {{'val' declared without an initial value}}
  if (size > 1) {
    // expected-note@-1 {{Assuming 'size' is > 1}}
    // expected-note@-2 {{Taking true branch}}
    for (unsigned j = 0; j < size + 1; ++j) {
      // expected-note@-1 {{Assuming 'j' == 0, 'size' == 4294967295}}
      // expected-note@-2 {{Loop condition is false. Execution continues on line 31}}
      init(&val);
    }
    assert((int) (val == 1)); // expected-warning{{The left operand of '==' is a garbage value}}
    // expected-note@-1 {{The left operand of '==' is a garbage value}}
  }
}
