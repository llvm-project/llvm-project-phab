// RUN: %clang_analyze_cc1 -analyzer-checker=debug.ExprInspection -verify %s

void clang_analyzer_dump(int x);
void clang_analyzer_printState();

int f();

void compare_different_symbol() {
  int x = f(), y = f();
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{((conj_$2{int}) - (conj_$5{int})) == 0}}
}

void compare_different_symbol_plus_left_int() {
  int x = f()+1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{((conj_$5{int}) - (conj_$2{int})) == 1}}
}

void compare_different_symbol_minus_left_int() {
  int x = f()-1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{((conj_$2{int}) - (conj_$5{int})) == 1}}
}

void compare_different_symbol_plus_right_int() {
  int x = f(), y = f()+2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 2}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{((conj_$2{int}) - (conj_$5{int})) == 2}}
}

void compare_different_symbol_minus_right_int() {
  int x = f(), y = f()-2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 2}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{((conj_$5{int}) - (conj_$2{int})) == 2}}
}

void compare_different_symbol_plus_left_plus_right_int() {
  int x = f()+2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{((conj_$5{int}) - (conj_$2{int})) == 1}}
}

void compare_different_symbol_plus_left_minus_right_int() {
  int x = f()+2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{((conj_$5{int}) - (conj_$2{int})) == 3}}
}

void compare_different_symbol_minus_left_plus_right_int() {
  int x = f()-2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{((conj_$2{int}) - (conj_$5{int})) == 3}}
}

void compare_different_symbol_minus_left_minus_right_int() {
  int x = f()-2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{((conj_$2{int}) - (conj_$5{int})) == 1}}
}

void compare_same_symbol() {
  int x = f(), y = x;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_left_int() {
  int x = f(), y = x;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_int() {
  int x = f(), y = x;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_right_int() {
  int x = f(), y = x+1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_right_int() {
  int x = f(), y = x-1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_left_plus_right_int() {
  int x = f(), y = x+1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_left_minus_right_int() {
  int x = f(), y = x-1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_plus_right_int() {
  int x = f(), y = x+1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_minus_right_int() {
  int x = f(), y = x-1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{1 S32b}}
}
