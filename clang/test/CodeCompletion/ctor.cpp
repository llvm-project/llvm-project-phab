struct foo {
  foo();
  ~foo();

  foo(const foo&) = delete;
  foo(foo&&) = delete;
  foo& operator=(const foo&) = delete;
  foo& operator=(foo&&) = delete;

  static bool faz;
  int far;
};

foo::foo() { }
// RUN: c-index-test -code-completion-at=%s:14:7 %s | FileCheck -check-prefix=CHECK-TRANSLATION-UNIT %s
// CHECK-TRANSLATION-UNIT:CXXConstructor:{TypedText foo}{LeftParen (}{RightParen )} (35)
// CHECK-TRANSLATION-UNIT:CXXConstructor:{TypedText foo}{LeftParen (}{Placeholder const foo &}{RightParen )} (35) (unavailable)
// CHECK-TRANSLATION-UNIT:CXXConstructor:{TypedText foo}{LeftParen (}{Placeholder foo &&}{RightParen )} (35) (unavailable)

namespace ns_foo {
struct foo {
  foo();
  ~foo();

  foo(const foo&) = delete;
  foo(foo&&) = delete;
  foo& operator=(const foo&) = delete;
  foo& operator=(foo&&) = delete;

  static bool faz;
  int far;
};

foo::foo() { }
// RUN: c-index-test -code-completion-at=%s:34:7 %s | FileCheck -check-prefix=CHECK-NAMESPACE %s
// CHECK-NAMESPACE: CXXConstructor:{TypedText foo}{LeftParen (}{RightParen )} (35)
// CHECK-NAMESPACE: CXXConstructor:{TypedText foo}{LeftParen (}{Placeholder const ns_foo::foo &}{RightParen )} (35) (unavailable)
// CHECK-NAMESPACE: CXXConstructor:{TypedText foo}{LeftParen (}{Placeholder ns_foo::foo &&}{RightParen )} (35) (unavailable)

}

int main() {
  foo baz;
  baz.far = 5;
  foo::faz = 42;
}
// RUN: c-index-test -code-completion-at=%s:44:7 %s | not grep "CXXConstructor"
// RUN: c-index-test -code-completion-at=%s:44:8 %s | not grep "CXXConstructor"
// RUN: c-index-test -code-completion-at=%s:45:8 %s | not grep "CXXConstructor"
// RUN: c-index-test -code-completion-at=%s:45:9 %s | not grep "CXXConstructor"
