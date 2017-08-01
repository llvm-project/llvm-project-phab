// RUN: %clang_cc1 -E %s > %t.src.cpp
// RUN: %clang_cc1 -E %s > %t.dst.cpp -DDEST
// RUN: clang-diff -m -no-compilation-database %t.src.cpp %t.dst.cpp | FileCheck %s

#ifndef DEST
namespace src {

void foo() {
  int x = 321;
}

void main() { foo(); };

const char *a = "foo";

typedef unsigned int nat;

int p = 1 * 2 * 3 * 4;
int squared = p * p;

class X {
  const char *foo(int i) {
    if (i == 0)
      return "foo";
    return 0;
  }

public:
  X(){};

  int id(int i) { return i; }
};
}

void f1() {{ (void) __func__;;; }}
#else
// CHECK: Match TranslationUnitDecl{{.*}} to TranslationUnitDecl
// CHECK: Match NamespaceDecl: src{{.*}} to NamespaceDecl: dst
namespace dst {
// CHECK-NOT: Match NamespaceDecl: src{{.*}} to NamespaceDecl: inner
namespace inner {
void foo() {
  // CHECK: Match IntegerLiteral: 321{{.*}} to IntegerLiteral: 322
  int x = 322;
}
}

// CHECK: Match DeclRefExpr: src::foo{{.*}} to DeclRefExpr: dst::inner::foo
void main() { inner::foo(); }

// CHECK: Match StringLiteral: foo{{.*}} to StringLiteral: foo
const char *b = "f" "o" "o";

// unsigned is canonicalized to unsigned int
// CHECK: Match TypedefDecl: src::nat;unsigned int;{{.*}} to TypedefDecl: dst::nat;unsigned int;
typedef unsigned nat;

// CHECK: Match VarDecl: src::p(int){{.*}} to VarDecl: dst::prod(double)
// CHECK: Update VarDecl: src::p(int){{.*}} to dst::prod(double)
// CHECK: Match BinaryOperator: *{{.*}} to BinaryOperator: *
double prod = 1 * 2 * 10;
// CHECK: Update DeclRefExpr
int squared = prod * prod;

class X {
  const char *foo(int i) {
    if (i == 0)
      return "Bar";
    // CHECK: Insert IfStmt{{.*}} into IfStmt
    // CHECK: Insert BinaryOperator: =={{.*}} into IfStmt
    else if (i == -1)
      return "foo";
    return 0;
  }
  X(){};
};
}

namespace {
// match with parents of different type
// CHECK: Match FunctionDecl: f1{{.*}} to FunctionDecl: (anonymous namespace)::f1
void f1() {{ (void) __func__;;; }}
}
#endif
// CHECK: Delete AccessSpecDecl: public
// CHECK: Delete CXXMethodDecl
