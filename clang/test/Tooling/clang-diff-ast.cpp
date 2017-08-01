// RUN: clang-diff -no-compilation-database -ast-dump %s -extra-arg='-std=c++11' | FileCheck %s
//
// This tests the getNodeValue function from Tooling/ASTDiff/ASTDiff.h

// CHECK: NamespaceDecl: test;(
namespace test {

// CHECK: FunctionDecl: test::f(
// CHECK: CompoundStmt(
void f() {
  // CHECK: VarDecl: i(int)(
  // CHECK: IntegerLiteral: 1
  auto i = 1;
  // CHECK: FloatingLiteral: 1.5(
  auto r = 1.5;
  // CHECK: CXXBoolLiteralExpr: true(
  auto b = true;
  // CHECK: CallExpr(
  // CHECK: DeclRefExpr: test::f(
  f();
  // CHECK: UnaryOperator: ++(
  ++i;
  // CHECK: BinaryOperator: =(
  i = i;
}

} // end namespace test

// CHECK: UsingDirectiveDecl: test(
using namespace test;

// CHECK: TypedefDecl: nat;unsigned int;(
typedef unsigned nat;
// CHECK: TypeAliasDecl: real;double;(
using real = double;

class Base {
};

// CHECK: CXXRecordDecl: X;X;(
class X : Base {
  int m;
  // CHECK: CXXMethodDecl: X::foo(const char *(int))(
  // CHECK: ParmVarDecl: i(int)(
  const char *foo(int i) {
    if (i == 0)
      // CHECK: StringLiteral: foo(
      return "foo";
    return 0;
  }

  // CHECK: AccessSpecDecl: public(
public:
  // CHECK: CXXConstructorDecl: X::X(void (char, int))Base,X::m,(
  X(char, int) : Base(), m(0) {
    // CHECK: MemberExpr: X::m(
    int x = m;
  }
};
