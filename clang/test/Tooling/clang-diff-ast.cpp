// RUN: clang-diff -ast-dump %s -- -std=c++11 | FileCheck %s


// CHECK: {{^}}TranslationUnitDecl(0)
// CHECK: {{^}} NamespaceDecl(1)
namespace test {

// CHECK: {{^}} FunctionDecl(2)
void f() {
  // CHECK: CompoundStmt(5)
  // CHECK: DeclStmt(6)
  // CHECK-NEXT: VarDecl
  // CHECK-NEXT: TypeLoc
  // CHECK-NEXT: QualType
  // CHECK-NEXT: IntegerLiteral
  auto i = 1;
  // CHECK: FloatingLiteral(15)
  auto r = 1.5;
  // CHECK: TypeLoc(18)
  // CHECK-NEXT: CXXBoolLiteralExpr
  bool b = true;
  // CHECK: CallExpr(20)
  // CHECK-NOT: ImplicitCastExpr
  // CHECK-NEXT: DeclRefExpr
  f();
  // CHECK: UnaryOperator(22)
  ++i;
  // CHECK: BinaryOperator(24)
  i = i;
}

} // end namespace test

// CHECK: UsingDirectiveDecl(27)
using namespace test;

// CHECK: TypedefDecl(28)
typedef unsigned nat;
// CHECK: TypeAliasDecl(30)
using real = double;

class Base {
};

// CHECK: CXXRecordDecl(33)
class X : Base {
  int m;
  // CHECK: CXXMethodDecl(37)
  // CHECK: ParmVarDecl(41)
  const char *foo(int i) {
    if (i == 0)
      // CHECK: StringLiteral(49)
      return "foo";
    // CHECK-NOT: ImplicitCastExpr
    return 0;
  }

  // CHECK: AccessSpecDecl(52)
public:
  int not_initialized;
  // CHECK: CXXConstructorDecl(55)
  // CHECK-NEXT: TypeLoc
  // CHECK-NEXT: TypeLoc
  // CHECK-NEXT: ParmVarDecl
  // CHECK-NEXT: TypeLoc
  // CHECK-NEXT: ParmVarDecl
  // CHECK-NEXT: TypeLoc
  // CHECK-NEXT: CXXCtorInitializer
  // CHECK-NEXT: TypeLoc
  // CHECK-NEXT: CXXConstructExpr
  // CHECK-NEXT: CXXCtorInitializer
  // CHECK-NEXT: IntegerLiteral
  X(char s, int) : Base(), m(0) {
    // CHECK-NEXT: CompoundStmt(67)
    // CHECK: MemberExpr(71)
    int x = m;
  }
  // CHECK: CXXConstructorDecl(73)
  // CHECK: CXXCtorInitializer(78)
  X(char s) : X(s, 4) {}
};

#define M (void)1
#define F(a, b) (void)a, b
void macros() {
  // CHECK: Macro(90)
  M;
  // two expressions, therefore it occurs twice
  // CHECK-NEXT: Macro(91)
  // CHECK-NEXT: Macro(92)
  F(1, 2);
}

#ifndef GUARD
#define GUARD
// CHECK-NEXT: NamespaceDecl(93)
namespace world {
// CHECK-NEXT: NamespaceDecl
#include "clang-diff-ast.cpp"
}
// CHECK: FunctionDecl(197)
void sentinel();
#endif

// CHECK: ClassTemplateDecl(200)
// CHECK-NEXT: TemplateTypeParmDecl
// CHECK-NEXT: QualType
// CHECK-NEXT: CXXRecordDecl
template <class T> class C {
  // CHECK-NEXT: FieldDecl
  // CHECK-NEXT: TypeLoc
  T t;
};

// CHECK-NEXT: CXXRecordDecl
// CHECK-NEXT: TypeLoc
// CHECK-NEXT: TemplateName
// CHECK-NEXT: TemplateArgument
class I : C<int> {};
