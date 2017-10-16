// RUN: not %clang_cc1 -std=c++11 -triple x86_64-linux-gnu -fms-extensions -ast-dump -ast-dump-filter Test %s | FileCheck -check-prefix CHECK -strict-whitespace %s

/* This test ensures that the AST is still complete, even for invalid code */

namespace TestInvalidSwithCondition {
int f(int x) {
  switch (_invalid_) {
  case 0:
    return 1;
  default:
    return 2;
  }
}
}

// CHECK: NamespaceDecl {{.*}} TestInvalidSwithCondition
// CHECK-NEXT: `-FunctionDecl
// CHECK-NEXT:   |-ParmVarDecl
// CHECK-NEXT:   `-CompoundStmt
// CHECK-NEXT:     `-SwitchStmt
// CHECK-NEXT:       |-<<<NULL>>>
// CHECK-NEXT:       |-<<<NULL>>>
// CHECK-NEXT:       |-OpaqueValueExpr
// CHECK-NEXT:       `-CompoundStmt
// CHECK-NEXT:         |-CaseStmt
// CHECK-NEXT:         | |-IntegerLiteral {{.*}} 'int' 0
// CHECK-NEXT:         | |-<<<NULL>>>
// CHECK-NEXT:         | `-ReturnStmt
// CHECK-NEXT:         |   `-IntegerLiteral {{.*}} 'int' 1
// CHECK-NEXT:         `-DefaultStmt
// CHECK-NEXT:           `-ReturnStmt
// CHECK-NEXT:             `-IntegerLiteral {{.*}} 'int' 2

namespace TestSwitchConditionNotIntegral {
int g(int *x) {
  switch (x) {
  case 0:
    return 1;
  default:
    return 2;
  }
}
}

// CHECK: NamespaceDecl {{.*}} TestSwitchConditionNotIntegral
// CHECK-NEXT: `-FunctionDecl
// CHECK-NEXT:   |-ParmVarDecl
// CHECK-NEXT:   `-CompoundStmt
// CHECK-NEXT:     `-SwitchStmt
// CHECK-NEXT:       |-<<<NULL>>>
// CHECK-NEXT:       |-<<<NULL>>>
// CHECK-NEXT:       |-ImplicitCastExpr
// CHECK-NEXT:       | `-DeclRefExpr {{.*}} 'x' 'int *'
// CHECK-NEXT:       `-CompoundStmt
// CHECK-NEXT:         |-CaseStmt
// CHECK-NEXT:         | |-IntegerLiteral {{.*}} 'int' 0
// CHECK-NEXT:         | |-<<<NULL>>>
// CHECK-NEXT:         | `-ReturnStmt
// CHECK-NEXT:         |   `-IntegerLiteral {{.*}} 'int' 1
// CHECK-NEXT:         `-DefaultStmt
// CHECK-NEXT:           `-ReturnStmt
// CHECK-NEXT:             `-IntegerLiteral {{.*}} 'int' 2

namespace TestSwitchInvalidCases {
int g(int x) {
  switch (x) {
  case _invalid_:
    return 1;
  case _invalid_:
    return 2;
  case x:
    return 3;
  default:
    return 4;
  default:
    return 5;
  }
}
}

// CHECK: NamespaceDecl {{.*}} TestSwitchInvalidCases
// CHECK-NEXT: `-FunctionDecl
// CHECK-NEXT:   |-ParmVarDecl
// CHECK-NEXT:   `-CompoundStmt
// CHECK-NEXT:     `-SwitchStmt
// CHECK-NEXT:       |-<<<NULL>>>
// CHECK-NEXT:       |-<<<NULL>>>
// CHECK-NEXT:       |-ImplicitCastExpr
// CHECK-NEXT:       | `-DeclRefExpr {{.*}}'x' 'int'
// CHECK-NEXT:       `-CompoundStmt
// CHECK-NEXT:         |-ReturnStmt
// CHECK-NEXT:         | `-IntegerLiteral {{.*}} 'int' 1
// CHECK-NEXT:         |-ReturnStmt
// CHECK-NEXT:         | `-IntegerLiteral {{.*}} 'int' 2
// CHECK-NEXT:         |-CaseStmt
// CHECK-NEXT:         | |-DeclRefExpr {{.*}} 'x' 'int'
// CHECK-NEXT:         | |-<<<NULL>>>
// CHECK-NEXT:         | `-ReturnStmt
// CHECK-NEXT:         |   `-IntegerLiteral {{.*}} 'int' 3
// CHECK-NEXT:         |-DefaultStmt
// CHECK-NEXT:         | `-ReturnStmt
// CHECK-NEXT:         |   `-IntegerLiteral {{.*}} 'int' 4
// CHECK-NEXT:         `-DefaultStmt
// CHECK-NEXT:           `-ReturnStmt
// CHECK-NEXT:             `-IntegerLiteral {{.*}} 'int' 5
