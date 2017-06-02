// RUN: %check_clang_tidy %s readability-redundant-keyword %t

extern void h();
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: redundant 'extern' keyword [readability-redundant-keyword]

void h2();

class Foo {
  inline int f() { return 0; }
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant 'inline' keyword [readability-redundant-keyword]
};

class X {
    int f() { return 0; }
    int g();
};

inline int X::g() { return 0; }

extern int g() { return 0; }

extern "C" void ok();
