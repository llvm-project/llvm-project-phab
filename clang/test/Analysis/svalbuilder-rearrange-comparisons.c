// RUN: %clang_analyze_cc1 -analyzer-checker=debug.ExprInspection -verify %s

void clang_analyzer_dump(int x);
void clang_analyzer_eval(int x);
void clang_analyzer_printState();

int f();

void compare_different_symbol_equal() {
  int x = f(), y = f();
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) == 0}}
}

void compare_different_symbol_plus_left_int_equal() {
  int x = f()+1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) == 1}}
}

void compare_different_symbol_minus_left_int_equal() {
  int x = f()-1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) == 1}}
}

void compare_different_symbol_plus_right_int_equal() {
  int x = f(), y = f()+2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 2}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) == 2}}
}

void compare_different_symbol_minus_right_int_equal() {
  int x = f(), y = f()-2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 2}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) == 2}}
}

void compare_different_symbol_plus_left_plus_right_int_equal() {
  int x = f()+2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) == 1}}
}

void compare_different_symbol_plus_left_minus_right_int_equal() {
  int x = f()+2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) == 3}}
}

void compare_different_symbol_minus_left_plus_right_int_equal() {
  int x = f()-2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) == 3}}
}

void compare_different_symbol_minus_left_minus_right_int_equal() {
  int x = f()-2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) == 1}}
}

void compare_same_symbol_equal() {
  int x = f(), y = x;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_left_int_equal() {
  int x = f(), y = x;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_int_equal() {
  int x = f(), y = x;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_right_int_equal() {
  int x = f(), y = x+1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_right_int_equal() {
  int x = f(), y = x-1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_left_plus_right_int_equal() {
  int x = f(), y = x+1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_left_minus_right_int_equal() {
  int x = f(), y = x-1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_plus_right_int_equal() {
  int x = f(), y = x+1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_minus_right_int_equal() {
  int x = f(), y = x-1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{1 S32b}}
}

void compare_different_symbol_less_or_equal() {
  int x = f(), y = f();
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) <= 0}}
}

void compare_different_symbol_plus_left_int_less_or_equal() {
  int x = f()+1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) >= 1}}
}

void compare_different_symbol_minus_left_int_less_or_equal() {
  int x = f()-1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) <= 1}}
}

void compare_different_symbol_plus_right_int_less_or_equal() {
  int x = f(), y = f()+2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 2}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) <= 2}}
}

void compare_different_symbol_minus_right_int_less_or_equal() {
  int x = f(), y = f()-2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 2}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) >= 2}}
}

void compare_different_symbol_plus_left_plus_right_int_less_or_equal() {
  int x = f()+2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) >= 1}}
}

void compare_different_symbol_plus_left_minus_right_int_less_or_equal() {
  int x = f()+2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) >= 3}}
}

void compare_different_symbol_minus_left_plus_right_int_less_or_equal() {
  int x = f()-2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) <= 3}}
}

void compare_different_symbol_minus_left_minus_right_int_less_or_equal() {
  int x = f()-2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) <= 1}}
}

void compare_same_symbol_less_or_equal() {
  int x = f(), y = x;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_left_int_less_or_equal() {
  int x = f(), y = x;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_int_less_or_equal() {
  int x = f(), y = x;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_right_int_less_or_equal() {
  int x = f(), y = x+1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_minus_right_int_less_or_equal() {
  int x = f(), y = x-1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_left_plus_right_int_less_or_equal() {
  int x = f(), y = x+1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_left_minus_right_int_less_or_equal() {
  int x = f(), y = x-1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_plus_right_int_less_or_equal() {
  int x = f(), y = x+1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_minus_left_minus_right_int_less_or_equal() {
  int x = f(), y = x-1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_different_symbol_less() {
  int x = f(), y = f();
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) < 0}}
}

void compare_different_symbol_plus_left_int_less() {
  int x = f()+1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) > 1}}
}

void compare_different_symbol_minus_left_int_less() {
  int x = f()-1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) < 1}}
}

void compare_different_symbol_plus_right_int_less() {
  int x = f(), y = f()+2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 2}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) < 2}}
}

void compare_different_symbol_minus_right_int_less() {
  int x = f(), y = f()-2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 2}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) > 2}}
}

void compare_different_symbol_plus_left_plus_right_int_less() {
  int x = f()+2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) > 1}}
}

void compare_different_symbol_plus_left_minus_right_int_less() {
  int x = f()+2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) > 3}}
}

void compare_different_symbol_minus_left_plus_right_int_less() {
  int x = f()-2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) < 3}}
}

void compare_different_symbol_minus_left_minus_right_int_less() {
  int x = f()-2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) < 1}}
}

void compare_same_symbol_less() {
  int x = f(), y = x;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_left_int_less() {
  int x = f(), y = x;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_int_less() {
  int x = f(), y = x;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_right_int_less() {
  int x = f(), y = x+1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_minus_right_int_less() {
  int x = f(), y = x-1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_left_plus_right_int_less() {
  int x = f(), y = x+1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_left_minus_right_int_less() {
  int x = f(), y = x-1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_plus_right_int_less() {
  int x = f(), y = x+1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_minus_left_minus_right_int_less() {
  int x = f(), y = x-1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void compare_different_symbol_equal_unsigned() {
  unsigned x = f(), y = f();
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) == 0}}
}

void compare_different_symbol_plus_left_int_equal_unsigned() {
  unsigned x = f()+1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) == 1}}
}

void compare_different_symbol_minus_left_int_equal_unsigned() {
  unsigned x = f()-1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) == 1}}
}

void compare_different_symbol_plus_right_int_equal_unsigned() {
  unsigned x = f(), y = f()+2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 2}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) == 2}}
}

void compare_different_symbol_minus_right_int_equal_unsigned() {
  unsigned x = f(), y = f()-2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 2}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) == 2}}
}

void compare_different_symbol_plus_left_plus_right_int_equal_unsigned() {
  unsigned x = f()+2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) == 1}}
}

void compare_different_symbol_plus_left_minus_right_int_equal_unsigned() {
  unsigned x = f()+2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) == 3}}
}

void compare_different_symbol_minus_left_plus_right_int_equal_unsigned() {
  unsigned x = f()-2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) == 3}}
}

void compare_different_symbol_minus_left_minus_right_int_equal_unsigned() {
  unsigned x = f()-2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) == 1}}
}

void compare_same_symbol_equal_unsigned() {
  unsigned x = f(), y = x;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_left_int_equal_unsigned() {
  unsigned x = f(), y = x;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_int_equal_unsigned() {
  unsigned x = f(), y = x;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_right_int_equal_unsigned() {
  unsigned x = f(), y = x+1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_right_int_equal_unsigned() {
  unsigned x = f(), y = x-1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_left_plus_right_int_equal_unsigned() {
  unsigned x = f(), y = x+1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_left_minus_right_int_equal_unsigned() {
  unsigned x = f(), y = x-1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_plus_right_int_equal_unsigned() {
  unsigned x = f(), y = x+1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_minus_right_int_equal_unsigned() {
  unsigned x = f(), y = x-1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x == y);
  // expected-warning@-1{{1 S32b}}
}

void compare_different_symbol_less_or_equal_unsigned() {
  unsigned x = f(), y = f();
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) <= 0}}
}

void compare_different_symbol_plus_left_int_less_or_equal_unsigned() {
  unsigned x = f()+1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) >= 1}}
}

void compare_different_symbol_minus_left_int_less_or_equal_unsigned() {
  unsigned x = f()-1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) <= 1}}
}

void compare_different_symbol_plus_right_int_less_or_equal_unsigned() {
  unsigned x = f(), y = f()+2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 2}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) <= 2}}
}

void compare_different_symbol_minus_right_int_less_or_equal_unsigned() {
  unsigned x = f(), y = f()-2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 2}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) >= 2}}
}

void compare_different_symbol_plus_left_plus_right_int_less_or_equal_unsigned() {
  unsigned x = f()+2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) >= 1}}
}

void compare_different_symbol_plus_left_minus_right_int_less_or_equal_unsigned() {
  unsigned x = f()+2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) >= 3}}
}

void compare_different_symbol_minus_left_plus_right_int_less_or_equal_unsigned() {
  unsigned x = f()-2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) <= 3}}
}

void compare_different_symbol_minus_left_minus_right_int_less_or_equal_unsigned() {
  unsigned x = f()-2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) <= 1}}
}

void compare_same_symbol_less_or_equal_unsigned() {
  unsigned x = f(), y = x;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_left_int_less_or_equal_unsigned() {
  unsigned x = f(), y = x;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_int_less_or_equal_unsigned() {
  unsigned x = f(), y = x;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_right_int_less_or_equal_unsigned() {
  unsigned x = f(), y = x+1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_minus_right_int_less_or_equal_unsigned() {
  unsigned x = f(), y = x-1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_left_plus_right_int_less_or_equal_unsigned() {
  unsigned x = f(), y = x+1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_left_minus_right_int_less_or_equal_unsigned() {
  unsigned x = f(), y = x-1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_plus_right_int_less_or_equal_unsigned() {
  unsigned x = f(), y = x+1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_minus_left_minus_right_int_less_or_equal_unsigned() {
  unsigned x = f(), y = x-1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x <= y);
  // expected-warning@-1{{1 S32b}}
}

void compare_different_symbol_less_unsigned() {
  unsigned x = f(), y = f();
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) < 0}}
}

void compare_different_symbol_plus_left_int_less_unsigned() {
  unsigned x = f()+1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) > 1}}
}

void compare_different_symbol_minus_left_int_less_unsigned() {
  unsigned x = f()-1, y = f();
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$5{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) < 1}}
}

void compare_different_symbol_plus_right_int_less_unsigned() {
  unsigned x = f(), y = f()+2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 2}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) < 2}}
}

void compare_different_symbol_minus_right_int_less_unsigned() {
  unsigned x = f(), y = f()-2;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 2}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) > 2}}
}

void compare_different_symbol_plus_left_plus_right_int_less_unsigned() {
  unsigned x = f()+2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) > 1}}
}

void compare_different_symbol_plus_left_minus_right_int_less_unsigned() {
  unsigned x = f()+2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$5{int})) - ((long) (conj_$2{int}))) > 3}}
}

void compare_different_symbol_minus_left_plus_right_int_less_unsigned() {
  unsigned x = f()-2, y = f()+1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) + 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) < 3}}
}

void compare_different_symbol_minus_left_minus_right_int_less_unsigned() {
  unsigned x = f()-2, y = f()-1;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 2}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$5{int}) - 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{(((long) (conj_$2{int})) - ((long) (conj_$5{int}))) < 1}}
}

void compare_same_symbol_less_unsigned() {
  unsigned x = f(), y = x;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_left_int_less_unsigned() {
  unsigned x = f(), y = x;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_int_less_unsigned() {
  unsigned x = f(), y = x;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_plus_right_int_less_unsigned() {
  unsigned x = f(), y = x+1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_minus_right_int_less_unsigned() {
  unsigned x = f(), y = x-1;
  clang_analyzer_dump(x); // expected-warning{{conj_$2{int}}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_left_plus_right_int_less_unsigned() {
  unsigned x = f(), y = x+1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_plus_left_minus_right_int_less_unsigned() {
  unsigned x = f(), y = x-1;
  ++x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void compare_same_symbol_minus_left_plus_right_int_less_unsigned() {
  unsigned x = f(), y = x+1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) + 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{1 S32b}}
}

void compare_same_symbol_minus_left_minus_right_int_less_unsigned() {
  unsigned x = f(), y = x-1;
  --x;
  clang_analyzer_dump(x); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(y); // expected-warning{{(conj_$2{int}) - 1}}
  clang_analyzer_dump(x < y);
  // expected-warning@-1{{0 S32b}}
}

void overflow(signed char n, signed char m) {
  if (n + 0 > m + 0) {
    clang_analyzer_eval(n - 126 == m + 3); // expected-warning{{UNKNOWN}}
  }
}
