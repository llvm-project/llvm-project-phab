// RUN: %check_clang_tidy %s misc-throw-with-noexcept %t

void f_throw_with_ne() noexcept(true) {
  {
    throw 5;
  }
}
// CHECK-MESSAGES: :[[@LINE-5]]:24: warning: In function declared no-throw here: [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-4]]:5: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-FIXES: void f_throw_with_ne() {

void f_noexcept_false() noexcept(false) {
  throw 5;
}

void f_decl_only() noexcept;

// Controversial example
void throw_and_catch() noexcept(true) {
  try {
    throw 5;
  } catch (...) {
  }
}
// CHECK-MESSAGES: :[[@LINE-6]]:24: warning: In function declared no-throw here: [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-5]]:5: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-FIXES: void throw_and_catch() {

void with_parens() noexcept((true)) {
	throw 42;
}
// CHECK-MESSAGES: :[[@LINE-3]]:20: warning: In function declared no-throw here: [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-3]]:2: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-FIXES: void with_parens() {


void with_parens_and_spaces() noexcept    (  (true)  ) {
	throw 42;
}
// CHECK-MESSAGES: :[[@LINE-3]]:31: warning: In function declared no-throw here: [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-3]]:2: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-FIXES: void with_parens_and_spaces() {

class Class {
  void InClass() const noexcept(true) {
    throw 42;
  }
};
// CHECK-MESSAGES: :[[@LINE-4]]:24: warning: In function declared no-throw here: [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-4]]:5: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-FIXES: void InClass() const {


void forward_declared() noexcept;
// CHECK-MESSAGES: :[[@LINE-1]]:25: warning: In function declared no-throw here: [misc-throw-with-noexcept]
// CHECK-FIXES: void forward_declared() ;

void forward_declared() noexcept {
	throw 42;
}
// CHECK-MESSAGES: :[[@LINE-3]]:25: warning: In function declared no-throw here: [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-3]]:2: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-FIXES: void forward_declared() {

void getFunction() noexcept {
  struct X {
    static void inner()
    {
        throw 42;
    }
  };
}

void dynamic_exception_spec() throw() {
  throw 42;
}
// CHECK-MESSAGES: :[[@LINE-3]]:31: warning: In function declared no-throw here: [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-3]]:3: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-FIXES: void dynamic_exception_spec() {

#define NOEXCEPT noexcept

void with_macro() NOEXCEPT {
  throw 42;
}
// CHECK-MESSAGES: :[[@LINE-3]]:19: warning: In function declared no-throw here: [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-3]]:3: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-FIXES: void with_macro() {
