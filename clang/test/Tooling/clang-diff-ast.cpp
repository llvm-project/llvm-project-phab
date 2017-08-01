// RUN: clang-diff -no-compilation-database -ast-dump %s | FileCheck %s
//
// This tests the getNodeValue function from Tooling/ASTDiff/ASTDiff.h

// CHECK: NamespaceDecl: test;(
namespace test {

// CHECK: FunctionDecl: f(
void f() {
  // CHECK: VarDecl: x(int)(
  // CHECK: IntegerLiteral: 1
  int x = 1;
  // CHECK: FloatingLiteral: 1.0(
  x = 1;
  // CHECK: CXXBoolLiteral: true(
  x = true;
  // CHECK: CallExpr(
  // CHECK: DeclRefExpr: f(
  f();
}

#if 0
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
#endif

}
