// RUN: clang-diff -dump-matches %S/Inputs/clang-diff-basic-src.cpp %s -s=200 -- -std=c++11 | FileCheck %s

// CHECK: Match TranslationUnitDecl(0) to TranslationUnitDecl(0)
// CHECK: Match NamespaceDecl(1) to NamespaceDecl(1)
namespace dst {
// CHECK-NOT: Match NamespaceDecl(1) to NamespaceDecl(2)
namespace inner {
void foo() {
  // CHECK: Match IntegerLiteral(9) to IntegerLiteral(10)
  int x = 322;
}
}

// CHECK: Match DeclRefExpr(15) to DeclRefExpr(16)
void main() { inner::foo(); }

// CHECK: Match StringLiteral(20) to StringLiteral(20)
const char *b = "f" "o" "o";

// unsigned is canonicalized to unsigned int
// CHECK: Match TypedefDecl
typedef unsigned nat;

// CHECK: Match VarDecl
// CHECK-NEXT: Update VarDecl
// CHECK-NEXT: Match TypeLoc
// CHECK-NEXT: Update TypeLoc
// CHECK-NEXT: Match BinaryOperator
double prod = 1 * 2 * 10;
// CHECK: Update DeclRefExpr
int squared = prod * prod;

class X {
  const char *foo(int i) {
    // CHECK: Insert IfStmt(43) into CompoundStmt(42)
    if (i == 0)
      return "Bar";
    // CHECK: Move IfStmt(49) into IfStmt
    else if (i == -1)
      return "foo";
    return 0;
  }
  X(){}
};
}

// CHECK: Move CompoundStmt(66) into CompoundStmt(65)
void m() { { int x = 0 + 0 + 0; } }
// CHECK: Update and Move IntegerLiteral(79) into BinaryOperator(77) at 1
int um = 1 + 7;

namespace {
// match with parents of different type
// CHECK: Match FunctionDecl(85) to FunctionDecl(81)
void f1() {{ (void) __func__;;; }}
}

// macros are always compared by their full textual value

#define M1 return 1 + 1
#define M2 return 2 * 2
#define F(a, b) return a + b;

int f2() {
  // CHECK: Match Macro(100) to Macro(96)
  M1;
  // CHECK: Update Macro(101)
  M2;
  // CHECK: Match Macro(102)
  F(1, /*b=*/1);
}

// CHECK: Match TemplateTypeParmDecl(105)
template <class Type, class U = int>
U visit(Type &t) {
  int x = t;
  return U();
}

void tmp() {
  long x;
  // CHECK: Match TemplateArgument(133)
  // CHECK: Update TypeLoc
  visit<long>(x);
}

// CHECK: Delete AccessSpecDecl
// CHECK: Delete CXXMethodDecl
