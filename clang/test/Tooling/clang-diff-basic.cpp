// RUN: clang-diff -dump-matches %S/Inputs/clang-diff-basic-src.cpp %s -- -std=c++11 | FileCheck %s

// CHECK: Match TranslationUnitDecl(0) to TranslationUnitDecl(0)
// CHECK: Match NamespaceDecl(1) to NamespaceDecl(1)
namespace dst {
// CHECK-NOT: Match NamespaceDecl(1) to NamespaceDecl(2)
namespace inner {
void foo() {
  // CHECK: Match IntegerLiteral(6) to IntegerLiteral(7)
  int x = 322;
}
}

// CHECK: Match DeclRefExpr(10) to DeclRefExpr(11)
void main() { inner::foo(); }

// CHECK: Match StringLiteral(13) to StringLiteral(13)
const char *b = "f" "o" "o";

// unsigned is canonicalized to unsigned int
// CHECK: Match TypedefDecl(14) to TypedefDecl(14)
typedef unsigned nat;

// CHECK: Match VarDecl(15)
// CHECK: Update VarDecl(15)
// CHECK: Match BinaryOperator(17)
double prod = 1 * 2 * 10;
// CHECK: Update DeclRefExpr(25)
int squared = prod * prod;

class X {
  const char *foo(int i) {
    // CHECK: Insert IfStmt(29) into CompoundStmt(28)
    if (i == 0)
      return "Bar";
    // CHECK: Move IfStmt(35) into IfStmt
    else if (i == -1)
      return "foo";
    return 0;
  }
  X(){}
};
}

// CHECK: Move CompoundStmt(48) into CompoundStmt(47)
void m() { { int x = 0 + 0 + 0; } }
// CHECK: Update and Move IntegerLiteral(59) into BinaryOperator(57) at 1
int um = 1 + 7;

namespace {
// match with parents of different type
// CHECK: Match FunctionDecl(70) to FunctionDecl(69)
void f1() {{ (void) __func__;;; }}
}

// macros are always compared by their full textual value

#define M1 return 1 + 1
#define M2 return 2 * 2
#define F(a, b) return a + b;

int f2() {
  // CHECK: Match Macro(72) to Macro(71)
  M1;
  // CHECK: Update Macro(73)
  M2;
  // CHECK: Match Macro(74)
  F(1, /*b=*/1);
}

// CHECK: Match TemplateTypeParmDecl(77)
template <class Type, class U = int>
U visit(Type &t) {
  int x = t;
  return U();
}

void tmp() {
  long x;
  // CHECK: Match TemplateArgument(93)
  visit<long>(x);
}

// CHECK: Delete AccessSpecDecl(39)
// CHECK: Delete CXXMethodDecl(42)
