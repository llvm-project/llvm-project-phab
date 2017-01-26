// RUN: %check_clang_tidy %s misc-throw-with-noexcept %t

void f_throw_with_ne() noexcept(true) {
  {
    throw 5;
  }
}
// CHECK-MESSAGES: :[[@LINE-3]]:5: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-6]]:24: note: in a function declared no-throw here:
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
// CHECK-MESSAGES: :[[@LINE-4]]:5: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-7]]:24: note: in a function declared no-throw here:
// CHECK-FIXES: void throw_and_catch() {

void with_parens() noexcept((true)) {
	throw 42;
}
// CHECK-MESSAGES: :[[@LINE-2]]:2: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-4]]:20: note: in a function declared no-throw here:
// CHECK-FIXES: void with_parens() {


void with_parens_and_spaces() noexcept    (  (true)  ) {
	throw 42;
}
// CHECK-MESSAGES: :[[@LINE-2]]:2: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-4]]:31: note: in a function declared no-throw here:
// CHECK-FIXES: void with_parens_and_spaces() {

class Class {
  void InClass() const noexcept(true) {
    throw 42;
  }
};
// CHECK-MESSAGES: :[[@LINE-3]]:5: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-5]]:24: note: in a function declared no-throw here:
// CHECK-FIXES: void InClass() const {


void forward_declared() noexcept;

void forward_declared() noexcept {
	throw 42;
}
// CHECK-MESSAGES: :[[@LINE-2]]:2: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-4]]:25: note: in a function declared no-throw here:
// CHECK-MESSAGES: :[[@LINE-7]]:25: note: in a function declared no-throw here:
// CHECK-FIXES: void forward_declared() ;
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
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-4]]:31: note: in a function declared no-throw here:
// CHECK-FIXES: void dynamic_exception_spec() {

#define NOEXCEPT noexcept

void with_macro() NOEXCEPT {
  throw 42;
}
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-4]]:19: note: in a function declared no-throw here:
// CHECK-FIXES: void with_macro() {

template<typename T> int template_function() noexcept {
  throw 42;
}
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
// CHECK-MESSAGES: :[[@LINE-4]]:46: note: in a function declared no-throw here:
// CHECK-FIXES: template<typename T> int template_function() {

template<typename T> class template_class {
  int throw_in_member() noexcept {
    throw 42;
  }
  // CHECK-MESSAGES: :[[@LINE-2]]:5: warning: 'throw' expression in a function declared with a non-throwing exception specification [misc-throw-with-noexcept]
  // CHECK-MESSAGES: :[[@LINE-4]]:25: note: in a function declared no-throw here:
  // CHECK-FIXES: int throw_in_member() {
};

void instantiations() {
  template_function<int>();
  template_function<float>();
  template_class<int> c1;
  template_class<float> c2;
}
