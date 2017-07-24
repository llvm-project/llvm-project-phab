// RUN: %clang_cc1 -fcxx-exceptions -fexceptions -analyze -analyzer-checker=debug.DumpCFG -analyzer-config cfg-scopes=true %s > %t 2>&1
// RUN: FileCheck --input-file=%t %s

class A {
public:
// CHECK:      [B1 (ENTRY)]
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
  A() {}

// CHECK:      [B1 (ENTRY)]
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
  ~A() {}

// CHECK:      [B2 (ENTRY)]
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: 1
// CHECK-NEXT:   3: return [B1.2];
// CHECK-NEXT:   4: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
  operator int() const { return 1; }
};

int getX();
extern const bool UV;

// CHECK:      [B2 (ENTRY)]
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4: a
// CHECK-NEXT:   5: [B1.4] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   6: const A &b = a;
// CHECK-NEXT:   7: A() (CXXConstructExpr, class A)
// CHECK-NEXT:   8: [B1.7] (BindTemporary)
// CHECK-NEXT:   9: [B1.8] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:  10: [B1.9]
// CHECK-NEXT:  11: const A &c = A();
// CHECK-NEXT:  12: [B1.11].~A() (Implicit destructor)
// CHECK-NEXT:  13: [B1.3].~A() (Implicit destructor)
// CHECK-NEXT:  14: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B0 (EXIT)]
void test_const_ref() {
  A a;
  const A& b = a;
  const A& c = A();
}

// CHECK:      [B2 (ENTRY)]
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A [2])
// CHECK-NEXT:   3: A a[2];
// CHECK-NEXT:   4:  (CXXConstructExpr, class A [0])
// CHECK-NEXT:   5: A b[0];
// CHECK-NEXT:   6: [B1.3].~A() (Implicit destructor)
// CHECK-NEXT:   7: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
void test_array() {
  A a[2];
  A b[0];
}

// CHECK:      [B2 (ENTRY)]
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   5:  (CXXConstructExpr, class A)
// CHECK-NEXT:   6: A c;
// CHECK-NEXT:   7:  (CXXConstructExpr, class A)
// CHECK-NEXT:   8: A d;
// CHECK-NEXT:   9: [B1.8].~A() (Implicit destructor)
// CHECK-NEXT:  10: [B1.6].~A() (Implicit destructor)
// CHECK-NEXT:  11:  (CXXConstructExpr, class A)
// CHECK-NEXT:  12: A b;
// CHECK-NEXT:  13: [B1.12].~A() (Implicit destructor)
// CHECK-NEXT:  14: [B1.3].~A() (Implicit destructor)
// CHECK-NEXT:  15: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:  16: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B0 (EXIT)]
void test_scope() {
  A a;
  { A c;
    A d;
  }
  A b;
}

// CHECK:      [B4 (ENTRY)]
// CHECK-NEXT:   Succs (1): B3
// CHECK:      [B1]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A c;
// CHECK-NEXT:   3: [B1.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B3.5].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:   6: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B3.5].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4:  (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A b;
// CHECK-NEXT:   6: UV
// CHECK-NEXT:   7: [B3.6] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B3.7]
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (2): B2 B1
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (2): B1 B2
void test_return() {
  A a;
  A b;
  if (UV) return;
  A c;
}

// CHECK:      [B5 (ENTRY)]
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   2: [B4.8].~A() (Implicit destructor)
// CHECK-NEXT:   3: [B4.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B2 B3
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: [B2.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   5: a
// CHECK-NEXT:   6: [B4.5] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   7: [B4.6] (CXXConstructExpr, class A)
// CHECK-NEXT:   8: A b = a;
// CHECK-NEXT:   9: b
// CHECK-NEXT:  10: [B4.9] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:  11: [B4.10].operator int
// CHECK-NEXT:  12: [B4.10]
// CHECK-NEXT:  13: [B4.12] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK-NEXT:  14: [B4.13] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: if [B4.14]
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (2): B3 B2
// CHECK:      [B0 (EXIT)]
void test_if_implicit_scope() {
  A a;
  if (A b = a)
    A c;
  else A c;
}

// CHECK:      [B8 (ENTRY)]
// CHECK-NEXT:   Succs (1): B7
// CHECK:      [B1]
// CHECK-NEXT:  l1:
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A c;
// CHECK-NEXT:   3: [B1.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B6.2].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B7.2].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (2): B2 B3
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A b;
// CHECK-NEXT:   3: [B2.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B6.4].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B3]
// CHECK-NEXT:   1: [B6.4].~A() (Implicit destructor)
// CHECK-NEXT:   T: goto l1;
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B4]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B4.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B4.2]
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (2): B3 B2
// CHECK:      [B5]
// CHECK-NEXT:   1: [B6.4].~A() (Implicit destructor)
// CHECK-NEXT:   2: [B6.2].~A() (Implicit destructor)
// CHECK-NEXT:   T: goto l0;
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B6
// CHECK:      [B6]
// CHECK-NEXT:  l0:
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A b;
// CHECK-NEXT:   3:  (CXXConstructExpr, class A)
// CHECK-NEXT:   4: A a;
// CHECK-NEXT:   5: UV
// CHECK-NEXT:   6: [B6.5] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B6.6]
// CHECK-NEXT:   Preds (2): B7 B5
// CHECK-NEXT:   Succs (2): B5 B4
// CHECK:      [B7]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A a;
// CHECK-NEXT:   Preds (1): B8
// CHECK-NEXT:   Succs (1): B6
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
void test_goto() {
  A a;
l0:
  A b;
  {
    A a;
    if (UV)
      goto l0;
    if (UV)
      goto l1;
    A b;
  }
l1:
  A c;
}

// CHECK:      [B9 (ENTRY)]
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   2: [B8.8].~A() (Implicit destructor)
// CHECK-NEXT:   3:  (CXXConstructExpr, class A)
// CHECK-NEXT:   4: A e;
// CHECK-NEXT:   5: [B1.4].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B8.3].~A() (Implicit destructor)
// CHECK-NEXT:   7: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B2 B5
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A d;
// CHECK-NEXT:   3: [B2.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B4.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B4.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B8.8].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B8.3].~A() (Implicit destructor)
// CHECK-NEXT:   6: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B4.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B4.5]
// CHECK-NEXT:   Preds (1): B8
// CHECK-NEXT:   Succs (2): B3 B2
// CHECK:      [B5]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A d;
// CHECK-NEXT:   3: [B5.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B7.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B7.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B8.8].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B8.3].~A() (Implicit destructor)
// CHECK-NEXT:   6: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B7]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B7.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B7.5]
// CHECK-NEXT:   Preds (1): B8
// CHECK-NEXT:   Succs (2): B6 B5
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   5: a
// CHECK-NEXT:   6: [B8.5] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   7: [B8.6] (CXXConstructExpr, class A)
// CHECK-NEXT:   8: A b = a;
// CHECK-NEXT:   9: b
// CHECK-NEXT:  10: [B8.9] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:  11: [B8.10].operator int
// CHECK-NEXT:  12: [B8.10]
// CHECK-NEXT:  13: [B8.12] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK-NEXT:  14: [B8.13] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: if [B8.14]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B7 B4
// CHECK:      [B0 (EXIT)]
void test_if_jumps() {
  A a;
  if (A b = a) {
    A c;
    if (UV) return;
    A d;
  } else {
    A c;
    if (UV) return;
    A d;
  }
  A e;
}

// CHECK:      [B12 (ENTRY)]
// CHECK-NEXT:   Succs (1): B11
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B2 B9
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (2): B3 B6
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (2): B4 B5
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: 'a'
// CHECK-NEXT:   3: char c = 'a';
// CHECK-NEXT:   4: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B3
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: a
// CHECK-NEXT:   3: [B5.2] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   4: [B5.3] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   5: ![B5.4]
// CHECK-NEXT:   T: if [B5.5]
// CHECK-NEXT:   Preds (2): B7 B8
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: 2
// CHECK-NEXT:   3: a
// CHECK-NEXT:   4: [B6.3] = [B6.2]
// CHECK-NEXT:   5: 3
// CHECK-NEXT:   6: b
// CHECK-NEXT:   7: [B6.6] = [B6.5]
// CHECK-NEXT:   8: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B7]
// CHECK-NEXT:   1: b
// CHECK-NEXT:   2: [B7.1] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   3: [B7.2] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   4: ![B7.3]
// CHECK-NEXT:   T: if [B8.4] && [B7.4]
// CHECK-NEXT:   Preds (1): B8
// CHECK-NEXT:   Succs (2): B6 B5
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: a
// CHECK-NEXT:   3: [B8.2] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   4: [B8.3] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: [B8.4] && ...
// CHECK-NEXT:   Preds (2): B10 B11
// CHECK-NEXT:   Succs (2): B7 B5
// CHECK:      [B9]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: 0
// CHECK-NEXT:   3: a
// CHECK-NEXT:   4: [B9.3] = [B9.2]
// CHECK-NEXT:   5: 1
// CHECK-NEXT:   6: b
// CHECK-NEXT:   7: [B9.6] = [B9.5]
// CHECK-NEXT:   8: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B10]
// CHECK-NEXT:   1: b
// CHECK-NEXT:   2: [B10.1] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   3: [B10.2] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: if [B11.4] && [B10.3]
// CHECK-NEXT:   Preds (1): B11
// CHECK-NEXT:   Succs (2): B9 B8
// CHECK:      [B11]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: a
// CHECK-NEXT:   3: [B11.2] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   4: [B11.3] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: [B11.4] && ...
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (2): B10 B8
// CHECK:      [B0 (EXIT)]
int a, b;
void text_if_with_and() {
  if (a && b) {
    a = 0;
    b = 1;
  } else if (a && !b) {
    a = 2;
    b = 3;
  } else if (!a)
    char c = 'a';
}

// CHECK:      [B6 (ENTRY)]
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(CXXConstructExpr)
// CHECK-NEXT:   2: [B4.5].~A() (Implicit destructor)
// CHECK-NEXT:   3: [B5.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeEnd(CXXConstructExpr)
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeBegin(WhileStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B4.5].~A() (Implicit destructor)
// CHECK-NEXT:   6: CFGScopeEnd(WhileStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(CXXConstructExpr)
// CHECK-NEXT:   2: a
// CHECK-NEXT:   3: [B4.2] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   4: [B4.3] (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A b = a;
// CHECK-NEXT:   6: b
// CHECK-NEXT:   7: [B4.6] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   8: [B4.7].operator int
// CHECK-NEXT:   9: [B4.7]
// CHECK-NEXT:  10: [B4.9] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK-NEXT:  11: [B4.10] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: while [B4.11]
// CHECK-NEXT:   Preds (2): B2 B5
// CHECK-NEXT:   Succs (2): B3 B1
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B0 (EXIT)]
void test_while_implicit_scope() {
  A a;
  while (A b = a)
    A c;
}

// CHECK:      [B12 (ENTRY)]
// CHECK-NEXT:   Succs (1): B11
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(CXXConstructExpr)
// CHECK-NEXT:   2: [B10.5].~A() (Implicit destructor)
// CHECK-NEXT:   3:  (CXXConstructExpr, class A)
// CHECK-NEXT:   4: A e;
// CHECK-NEXT:   5: [B1.4].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B11.3].~A() (Implicit destructor)
// CHECK-NEXT:   7: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B8 B10
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeEnd(CXXConstructExpr)
// CHECK-NEXT:   Preds (2): B3 B6
// CHECK-NEXT:   Succs (1): B10
// CHECK:      [B3]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A d;
// CHECK-NEXT:   3: [B3.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B10.5].~A() (Implicit destructor)
// CHECK-NEXT:   6: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B10.5].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B11.3].~A() (Implicit destructor)
// CHECK-NEXT:   6: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B5]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B5.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B5.2]
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   3: [B10.5].~A() (Implicit destructor)
// CHECK-NEXT:   4: CFGScopeEnd(ContinueStmt)
// CHECK-NEXT:   T: continue;
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B7]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B7.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B7.2]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B6 B5
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   3: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B9]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B9.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B9.5]
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (2): B8 B7
// CHECK:      [B10]
// CHECK-NEXT:   1: CFGScopeBegin(CXXConstructExpr)
// CHECK-NEXT:   2: a
// CHECK-NEXT:   3: [B10.2] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   4: [B10.3] (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A b = a;
// CHECK-NEXT:   6: b
// CHECK-NEXT:   7: [B10.6] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   8: [B10.7].operator int
// CHECK-NEXT:   9: [B10.7]
// CHECK-NEXT:  10: [B10.9] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK-NEXT:  11: [B10.10] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: while [B10.11]
// CHECK-NEXT:   Preds (2): B2 B11
// CHECK-NEXT:   Succs (2): B9 B1
// CHECK:      [B11]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (1): B10
// CHECK:      [B0 (EXIT)]
void test_while_jumps() {
  A a;
  while (A b = a) {
    A c;
    if (UV) break;
    if (UV) continue;
    if (UV) return;
    A d;
  }
  A e;
}

// CHECK:      [B12 (ENTRY)]
// CHECK-NEXT:   Succs (1): B11
// CHECK:      [B1]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A d;
// CHECK-NEXT:   3: [B1.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B11.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B8 B2
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B2.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: do ... while [B2.2]
// CHECK-NEXT:   Preds (2): B3 B6
// CHECK-NEXT:   Succs (2): B10 B1
// CHECK:      [B3]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A c;
// CHECK-NEXT:   3: [B3.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B11.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B5]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B5.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B5.2]
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   3: CFGScopeEnd(ContinueStmt)
// CHECK-NEXT:   T: continue;
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B7]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B7.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B7.2]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B6 B5
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   3: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B9]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A b;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B9.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B9.5]
// CHECK-NEXT:   Preds (2): B10 B11
// CHECK-NEXT:   Succs (2): B8 B7
// CHECK:      [B10]
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B9
// CHECK:      [B11]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (1): B9
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (2): B1 B4
void test_do_jumps() {
  A a;
  do {
    A b;
    if (UV) break;
    if (UV) continue;
    if (UV) return;
    A c;
  } while (UV);
  A d;
}

// CHECK:      [B6 (ENTRY)]
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   2: [B4.5].~A() (Implicit destructor)
// CHECK-NEXT:   3: [B5.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeBegin(ForStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B4.5].~A() (Implicit destructor)
// CHECK-NEXT:   6: CFGScopeEnd(ForStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   2: a
// CHECK-NEXT:   3: [B4.2] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   4: [B4.3] (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A b = a;
// CHECK-NEXT:   6: b
// CHECK-NEXT:   7: [B4.6] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   8: [B4.7].operator int
// CHECK-NEXT:   9: [B4.7]
// CHECK-NEXT:  10: [B4.9] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK-NEXT:  11: [B4.10] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: for (...; [B4.11]; )
// CHECK-NEXT:   Preds (2): B2 B5
// CHECK-NEXT:   Succs (2): B3 B1
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B0 (EXIT)]
void test_for_implicit_scope() {
  for (A a; A b = a; )
    A c;
}

// CHECK:      [B12 (ENTRY)]
// CHECK-NEXT:   Succs (1): B11
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   2: [B10.5].~A() (Implicit destructor)
// CHECK-NEXT:   3: [B11.5].~A() (Implicit destructor)
// CHECK-NEXT:   4:  (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A f;
// CHECK-NEXT:   6: [B1.5].~A() (Implicit destructor)
// CHECK-NEXT:   7: [B11.3].~A() (Implicit destructor)
// CHECK-NEXT:   8: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B8 B10
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   Preds (2): B3 B6
// CHECK-NEXT:   Succs (1): B10
// CHECK:      [B3]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A e;
// CHECK-NEXT:   3: [B3.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B10.5].~A() (Implicit destructor)
// CHECK-NEXT:   6: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B10.5].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B11.5].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B11.3].~A() (Implicit destructor)
// CHECK-NEXT:   7: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B5]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B5.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B5.2]
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   3: CFGScopeEnd(ContinueStmt)
// CHECK-NEXT:   T: continue;
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B7]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B7.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B7.2]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B6 B5
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   3: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B9]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A d;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B9.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B9.5]
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (2): B8 B7
// CHECK:      [B10]
// CHECK-NEXT:   1: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   2: b
// CHECK-NEXT:   3: [B10.2] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   4: [B10.3] (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A c = b;
// CHECK-NEXT:   6: c
// CHECK-NEXT:   7: [B10.6] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   8: [B10.7].operator int
// CHECK-NEXT:   9: [B10.7]
// CHECK-NEXT:  10: [B10.9] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK-NEXT:  11: [B10.10] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: for (...; [B10.11]; )
// CHECK-NEXT:   Preds (2): B2 B11
// CHECK-NEXT:   Succs (2): B9 B1
// CHECK:      [B11]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4:  (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A b;
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (1): B10
// CHECK:      [B0 (EXIT)]
void test_for_jumps() {
  A a;
  for (A b; A c = b; ) {
    A d;
    if (UV) break;
    if (UV) continue;
    if (UV) return;
    A e;
  }
  A f;
}

// CHECK:      [B7 (ENTRY)]
// CHECK-NEXT:   Succs (1): B6
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   2: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   3: int unused2;
// CHECK-NEXT:   4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   5: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B4 B5
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: i
// CHECK-NEXT:   2: ++[B2.1]
// CHECK-NEXT:   3: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   3: int unused1;
// CHECK-NEXT:   4: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   2: i
// CHECK-NEXT:   3: [B5.2] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   4: 3
// CHECK-NEXT:   5: [B5.3] < [B5.4]
// CHECK-NEXT:   T: for (...; [B5.5]; ...)
// CHECK-NEXT:   Preds (2): B2 B6
// CHECK-NEXT:   Succs (2): B4 B1
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: 0
// CHECK-NEXT:   3: int i = 0;
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B0 (EXIT)]
void test_for_compound_and_break() {
  for (int i = 0; i < 3; ++i) {
    {
      int unused1;
      break;
    }
  }
  {
    int unused2;
  }
}

// CHECK:      [B6 (ENTRY)]
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B1]
// CHECK-NEXT:   1: [B5.3].~A() (Implicit destructor)
// CHECK-NEXT:   2: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: __begin
// CHECK-NEXT:   2: [B2.1] (ImplicitCastExpr, LValueToRValue, class A *)
// CHECK-NEXT:   3: __end
// CHECK-NEXT:   4: [B2.3] (ImplicitCastExpr, LValueToRValue, class A *)
// CHECK-NEXT:   5: [B2.2] != [B2.4]
// CHECK-NEXT:   T: for (auto &i : [B5.4]) {
// CHECK:         [B4.12];
// CHECK-NEXT:}
// CHECK-NEXT:   Preds (2): B3 B5
// CHECK-NEXT:   Succs (2): B4 B1
// CHECK:      [B3]
// CHECK-NEXT:   1: __begin
// CHECK-NEXT:   2: ++[B3.1]
// CHECK-NEXT:   3: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   2: CFGScopeBegin(CXXForRangeStmt)
// CHECK-NEXT:   3: __begin
// CHECK-NEXT:   4: [B4.3] (ImplicitCastExpr, LValueToRValue, class A *)
// CHECK-NEXT:   5: *[B4.4]
// CHECK-NEXT:   6: auto &i = *__begin;
// CHECK-NEXT:   7: operator=
// CHECK-NEXT:   8: [B4.7] (ImplicitCastExpr, FunctionToPointerDecay, class A &(*)(const class A &) throw())
// CHECK-NEXT:   9: i
// CHECK-NEXT:  10: b
// CHECK-NEXT:  11: [B4.10] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:  12: [B4.9] = [B4.11] (OperatorCall)
// CHECK-NEXT:  13: CFGScopeEnd(CXXForRangeStmt)
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B3
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A [10])
// CHECK-NEXT:   3: A a[10];
// CHECK-NEXT:   4: a
// CHECK-NEXT:   5: auto &&__range = a;
// CHECK-NEXT:   6: __range
// CHECK-NEXT:   7: [B5.6] (ImplicitCastExpr, ArrayToPointerDecay, class A *)
// CHECK-NEXT:   8: 10L
// CHECK-NEXT:   9: [B5.7] + [B5.8]
// CHECK-NEXT:  10: auto __end = __range + 10L;
// CHECK-NEXT:  11: __range
// CHECK-NEXT:  12: [B5.11] (ImplicitCastExpr, ArrayToPointerDecay, class A *)
// CHECK-NEXT:  13: auto __begin = __range;
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B0 (EXIT)]
void test_range_for(A &b) {
  A a[10];
  for (auto &i : a)
    i = b;
}

// CHECK:      [B14 (ENTRY)]
// CHECK-NEXT:   Succs (1): B13
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   2: [B13.3].~A() (Implicit destructor)
// CHECK-NEXT:   3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: i
// CHECK-NEXT:   2: ++[B2.1]
// CHECK-NEXT:   3: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   Preds (2): B3 B4
// CHECK-NEXT:   Succs (1): B12
// CHECK:      [B3]
// CHECK-NEXT:   1: j
// CHECK-NEXT:   2: ++[B3.1]
// CHECK-NEXT:   3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: CFGScopeEnd(ContinueStmt)
// CHECK-NEXT:   T: continue;
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeEnd(CXXConstructExpr)
// CHECK-NEXT:   2: [B10.5].~A() (Implicit destructor)
// CHECK-NEXT:   3: i
// CHECK-NEXT:   4: [B5.3] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   5: 3
// CHECK-NEXT:   6: [B5.4] > [B5.5]
// CHECK-NEXT:   T: if [B5.6]
// CHECK-NEXT:   Preds (2): B8 B10
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeEnd(CXXConstructExpr)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B10
// CHECK:      [B7]
// CHECK-NEXT:   1: j
// CHECK-NEXT:   2: ++[B7.1]
// CHECK-NEXT:   3: [B10.5].~A() (Implicit destructor)
// CHECK-NEXT:   4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (1): B6
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B9]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: int unused2;
// CHECK-NEXT:   3: j
// CHECK-NEXT:   4: [B9.3] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   5: 13
// CHECK-NEXT:   6: [B9.4] > [B9.5]
// CHECK-NEXT:   T: if [B9.6]
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (2): B8 B7
// CHECK:      [B10]
// CHECK-NEXT:   1: CFGScopeBegin(CXXConstructExpr)
// CHECK-NEXT:   2: a
// CHECK-NEXT:   3: [B10.2] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   4: [B10.3] (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A b = a;
// CHECK-NEXT:   6: b
// CHECK-NEXT:   7: [B10.6] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   8: [B10.7].operator int
// CHECK-NEXT:   9: [B10.7]
// CHECK-NEXT:  10: [B10.9] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK-NEXT:  11: [B10.10] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: while [B10.11]
// CHECK-NEXT:   Preds (2): B6 B11
// CHECK-NEXT:   Succs (2): B9 B5
// CHECK:      [B11]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: int unused1;
// CHECK-NEXT:   3: i
// CHECK-NEXT:   4: [B11.3] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   5: int j = i;
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (1): B10
// CHECK:      [B12]
// CHECK-NEXT:   1: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   2: i
// CHECK-NEXT:   3: [B12.2] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   4: 10
// CHECK-NEXT:   5: [B12.3] < [B12.4]
// CHECK-NEXT:   T: for (...; [B12.5]; ...)
// CHECK-NEXT:   Preds (2): B2 B13
// CHECK-NEXT:   Succs (2): B11 B1
// CHECK:      [B13]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4: 0
// CHECK-NEXT:   5: int i = 0;
// CHECK-NEXT:   Preds (1): B14
// CHECK-NEXT:   Succs (1): B12
// CHECK:      [B0 (EXIT)
void test_nested_loops() {
  A a;
  for (int i = 0; i < 10; ++i) {
    int unused1;
    int j = i;
    while (A b = a) {
      int unused2;
      if (j > 13)
        break; // Break from while loop
      ++j;
    }
    if (i > 3)
      continue; // Continue for loop
    ++j;
  }
}
