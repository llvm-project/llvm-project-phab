// RUN: clang-diff -ast-dump %s -- -std=c++11 | FileCheck %s


// CHECK: {{^}}TranslationUnitDecl(0)
// CHECK: {{^}} NamespaceDecl(1)
namespace test {

// CHECK: {{^}} FunctionDecl(2)
// CHECK: CompoundStmt(3)
void f() {
  // CHECK: DeclStmt(4)
  // CHECK: VarDecl(5)
  // CHECK: IntegerLiteral(6)
  auto i = 1;
  // CHECK: FloatingLiteral(9)
  auto r = 1.5;
  // CHECK: CXXBoolLiteralExpr(12)
  auto b = true;
  // CHECK: CallExpr(13)
  // CHECK-NOT: ImplicitCastExpr
  // CHECK: DeclRefExpr(14)
  f();
  // CHECK: UnaryOperator(15)
  ++i;
  // CHECK: BinaryOperator(17)
  i = i;
}

} // end namespace test

// CHECK: UsingDirectiveDecl(20)
using namespace test;

// CHECK: TypedefDecl(21)
typedef unsigned nat;
// CHECK: TypeAliasDecl(22)
using real = double;

class Base {
};

// CHECK: CXXRecordDecl(23)
class X : Base {
  int m;
  // CHECK: CXXMethodDecl(26)
  // CHECK: ParmVarDecl(27)
  const char *foo(int i) {
    if (i == 0)
      // CHECK: StringLiteral(34)
      return "foo";
    // CHECK-NOT: ImplicitCastExpr
    return 0;
  }

  // CHECK: AccessSpecDecl(37)
public:
  int not_initialized;
  // CHECK: CXXConstructorDecl(39)
  // CHECK-NEXT: ParmVarDecl(40)
  // CHECK-NEXT: ParmVarDecl(41)
  // CHECK-NEXT: CXXCtorInitializer(42)
  // CHECK-NEXT: CXXConstructExpr(43)
  // CHECK-NEXT: CXXCtorInitializer(44)
  // CHECK-NEXT: IntegerLiteral(45)
  X(char s, int) : Base(), m(0) {
    // CHECK-NEXT: CompoundStmt(46)
    // CHECK: MemberExpr(49)
    int x = m;
  }
  // CHECK: CXXConstructorDecl(51)
  // CHECK: CXXCtorInitializer(53)
  X(char s) : X(s, 4) {}
};

#define M (void)1
#define F(a, b) (void)a, b
void macros() {
  // CHECK: Macro(60)
  M;
  // two expressions, therefore it occurs twice
  // CHECK-NEXT: Macro(61)
  // CHECK-NEXT: Macro(62)
  F(1, 2);
}

#ifndef GUARD
#define GUARD
// CHECK-NEXT: NamespaceDecl(63)
namespace world {
// nodes from other files are excluded, there should be no output here
#include "clang-diff-ast.cpp"
}
// CHECK-NEXT: FunctionDecl(64)
void sentinel();
#endif

// CHECK-NEXT: ClassTemplateDecl(65)
// CHECK-NEXT: TemplateTypeParmDecl(66)
// CHECK-NEXT: CXXRecordDecl(67)
template <class T> class C {
  // CHECK-NEXT: FieldDecl(68)
  T t;
};

// CHECK-NEXT: CXXRecordDecl(69)
// CHECK-NEXT: TemplateName(70)
// CHECK-NEXT: TemplateArgument(71)
class I : C<int> {};
