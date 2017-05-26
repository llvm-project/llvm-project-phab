// RUN: %check_clang_tidy %s misc-exception-escape %t -- -extra-arg=-std=c++11 -config="{CheckOptions: [{key: misc-exception-escape.IgnoredExceptions, value: 'ignored1,ignored2'}, {key: misc-exception-escape.EnabledFunctions, value: 'enabled1,enabled2,enabled3'}]}" --

struct throwing_destructor {
  ~throwing_destructor() {
    // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function '~throwing_destructor' throws
    throw 1;
  }
};

struct throwing_move_constructor {
  throwing_move_constructor(throwing_move_constructor&&) {
    // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function 'throwing_move_constructor' throws
    throw 1;
  }
};

struct throwing_move_assignment {
  throwing_move_assignment& operator=(throwing_move_assignment&&) {
    // CHECK-MESSAGES: :[[@LINE-1]]:29: warning: function 'operator=' throws
    throw 1;
  }
};

void throwing_noexcept() noexcept {
    // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'throwing_noexcept' throws
  throw 1;
}

void throwing_throw_nothing() throw() {
    // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'throwing_throw_nothing' throws
  throw 1;
}

void throw_and_catch() noexcept {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'throw_and_catch' throws
  try {
    throw 1;
  } catch(int &) {
  }
}

void throw_and_catch_some() noexcept {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'throw_and_catch_some' throws
  try {
    throw 1;
    throw 1.1;
  } catch(int &) {
  }
}

void throw_and_catch_each() noexcept {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'throw_and_catch_each' throws
  try {
    throw 1;
    throw 1.1;
  } catch(int &) {
  } catch(double &) {
  }
}

void throw_and_catch_all() noexcept {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'throw_and_catch_all' throws
  try {
    throw 1;
    throw 1.1;
  } catch(...) {
  }
}

void throw_and_rethrow() noexcept {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'throw_and_rethrow' throws
  try {
    throw 1;
  } catch(int &) {
    throw;
  }
}

void throw_catch_throw() noexcept {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'throw_catch_throw' throws
  try {
    throw 1;
  } catch(int &) {
    throw 2;
  }
}

void throw_catch_rethrow_the_rest() noexcept {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'throw_catch_rethrow_the_rest' throws
  try {
    throw 1;
    throw 1.1;
  } catch(int &) {
  } catch(...) {
    throw;
  }
}

class base {};
class derived: public base {};

void throw_derived_catch_base() noexcept {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'throw_derived_catch_base' throws
  try {
    throw derived();
  } catch(base &) {
  }
}

void try_nested_try() noexcept {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'try_nested_try' throws
  try {
    try {
      throw 1;
      throw 1.1;
    } catch(int &) {
    }
  } catch(double &) {
  }
}

void bad_try_nested_try() noexcept {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'bad_try_nested_try' throws
  try {
    throw 1;
    try {
      throw 1.1;
    } catch(int &) {
    }
  } catch(double &) {
  }
}

void try_nested_catch() noexcept {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'try_nested_catch' throws
  try {
    try {
      throw 1;
    } catch(int &) {
      throw 1.1;
    }
  } catch(double &) {
  }
}

void catch_nested_try() noexcept {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'catch_nested_try' throws
  try {
    throw 1;
  } catch(int &) {
    try {
      throw 1; 
    } catch(int &) {
    }
  }
}

void bad_catch_nested_try() noexcept {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'bad_catch_nested_try' throws
  try {
    throw 1;
  } catch(int &) {
    try {
      throw 1.1; 
    } catch(int &) {
    }
  } catch(double &) {
  }
}

void implicit_int_thrower() {
  throw 1;
}

void explicit_int_thrower() throw(int);

void indirect_implicit() noexcept {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'indirect_implicit' throws
  implicit_int_thrower();
}

void indirect_explicit() noexcept {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'indirect_explicit' throws
  explicit_int_thrower();
}

void indirect_catch() noexcept {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'indirect_catch' throws
  try {
    implicit_int_thrower();
  } catch(int&) {
  }
}

void swap(int&, int&) {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'swap' throws
  throw 1;
}

class bad_alloc {};

void alloc() {
  throw bad_alloc();
}

void allocator() noexcept {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'allocator' throws
  alloc();
}

void enabled1() {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'enabled1' throws
  throw 1;
}

void enabled2() {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'enabled2' throws
  enabled1();
}

void enabled3() {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'enabled3' throws
  try {
    enabled1();
  } catch(...) {
  }
}

class ignored1 {};
class ignored2 {};

void this_does_not_count() noexcept {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'this_does_not_count' throws
  throw ignored1();
}

void this_does_not_count_either() noexcept {
  // CHECK-MESSAGES-NOT: :[[@LINE-1]]:6: warning: function 'this_does_not_count_either' throws
  try {
    throw 1;
    throw ignored2();
  } catch(int &) {
  }
}

void this_counts() noexcept {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'this_counts' throws
  throw 1;
  throw ignored1();
}

int main() {
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: function 'main' throws
  throw 1;
  return 0;
}
