// RUN: %clang_cc1 %s -triple=i686-pc-win32 -fsyntax-only -verify -fms-extensions -Wunreachable-code
// RUN: %clang_cc1 %s -triple=i686-pc-win32 -fsyntax-only -verify -fms-extensions -Wunreachable-code -x c++ -fcxx-exceptions -DWITH_THROW

void f();

void g1() {
  __try {
    f();
    __leave;
    f();  // expected-warning{{will never be executed}}
  } __except (1) {
    f();
  }

  // Completely empty.
  __try {
  } __except (1) {
  }

  __try {
    f();
    return;
  } __except (1) { // Filter expression should not be marked as unreachable.
    // Empty __except body.
  }
}

void g2() {
  __try {
    // Nested __try.
    __try {
      f();
      __leave;
      f(); // expected-warning{{will never be executed}}
    } __except (2) {
    }
    f();
    __leave;
    f(); // expected-warning{{will never be executed}}
  } __except (1) {
    f();
  }
}

#if defined(WITH_THROW)
void g3() {
  __try {
    __try {
      throw 1;
    } __except (1) {
      __leave; // should exit outer try
    }
    f(); // expected-warning{{never be executed}}
  } __except (1) {
  }
}
#endif

void g4() {
  __try {
    f();
    __leave;
    f();  // expected-warning{{will never be executed}}
  } __finally {
    f();
  }

  // Completely empty.
  __try {
  } __finally {
  }

  __try {
    f();
    return;  // Shouldn't make __finally body unreachable.
  } __finally {
    f();
  }
}

void g5() {
  __try {
    // Nested __try.
    __try {
      f();
      __leave;
      f(); // expected-warning{{will never be executed}}
    } __finally {
    }
    f();
    __leave;
    f(); // expected-warning{{will never be executed}}
  } __finally {
    f();
  }
}

// Jumps out of __finally have undefined behavior, but they're useful for
// checking that the generated CFG looks right.

#if defined(WITH_THROW)
void g6() {
  __try {
    __try {
      throw 1;
    } __finally {
      __leave; // should exit outer try expected-warning {{jump out of __finally block has undefined behavior}}
    }
    f(); // expected-warning{{never be executed}}
  } __finally {
  }
}
#endif

void g7() {
  f();
  __try {
  } __finally {
    return; // expected-warning {{jump out of __finally block has undefined behavior}}
  }
  f(); // expected-warning{{never be executed}}
}
