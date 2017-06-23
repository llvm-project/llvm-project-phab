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

// CHECK:      [B3 (ENTRY)]
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: 1
// CHECK-NEXT:   3: return [B1.2];
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B1
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B0 (EXIT)]
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
// CHECK-NEXT:  12: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:  13: [B1.11].~A() (Implicit destructor)
// CHECK-NEXT:  14: [B1.3].~A() (Implicit destructor)
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
// CHECK-NEXT:   6: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   7: [B1.3].~A() (Implicit destructor)
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
// CHECK-NEXT:   9: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   10: [B1.8].~A() (Implicit destructor)
// CHECK-NEXT:   11: [B1.6].~A() (Implicit destructor)
// CHECK-NEXT:   12:  (CXXConstructExpr, class A)
// CHECK:       13: A b;
// CHECK:       14: CFGScopeEnd(CompoundStmt)
// CHECK:       15: [B1.13].~A() (Implicit destructor)
// CHECK:       16: [B1.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
void test_scope() {
  A a;
  { A c;
    A d;
  }
  A b;
}

// CHECK:      [B5 (ENTRY)]
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B1]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A c;
// CHECK-NEXT:   3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   4: [B1.2].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B4.5].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B4.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B4.5].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B4.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B3
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4:  (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A b;
// CHECK-NEXT:   6: UV
// CHECK-NEXT:   7: [B4.6] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B4.7]
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (2): B2 B1
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (2): B1 B3
void test_return() {
  A a;
  A b;
  if (UV) return;
  A c;
}

// CHECK:      [B7 (ENTRY)]
// CHECK-NEXT:   Succs (1): B6
// CHECK:      [B1]
// CHECK-NEXT:   1: [B6.7].~A() (Implicit destructor)
// CHECK-NEXT:   2: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   3: [B6.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (2): B2 B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:   2: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: [B5.3].~A() (Implicit destructor)
// CHECK-NEXT:   2: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4: a
// CHECK-NEXT:   5: [B6.4] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   6: [B6.5] (CXXConstructExpr, class A)
// CHECK-NEXT:   7: A b = a;
// CHECK-NEXT:   8: b
// CHECK-NEXT:   9: [B6.8] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:  10: [B6.9].operator int
// CHECK-NEXT:  11: [B6.9]
// CHECK-NEXT:  12: [B6.11] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK-NEXT:  13: [B6.12] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: if [B6.13]
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (2): B5 B3
// CHECK:      [B0 (EXIT)]
void test_if_implicit_scope() {
  A a;
  if (A b = a)
    A c;
  else A c;
}

// CHECK:      [B11 (ENTRY)]
// CHECK-NEXT:   Succs (1): B10
// CHECK:      [B1]
// CHECK-NEXT:   1: [B10.7].~A() (Implicit destructor)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A e;
// CHECK-NEXT:   4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   5: [B1.3].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B10.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (2): B2 B6
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A d;
// CHECK-NEXT:   3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   4: [B2.2].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B5.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B5.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B10.7].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B10.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B5.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B5.5]
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (2): B3 B2
// CHECK:      [B6]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A d;
// CHECK-NEXT:   3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   4: [B6.2].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B7]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B10.7].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B10.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B9]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B9.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B9.5]
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (2): B7 B6
// CHECK:      [B10]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4: a
// CHECK-NEXT:   5: [B10.4] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   6: [B10.5] (CXXConstructExpr, class A)
// CHECK-NEXT:   7: A b = a;
// CHECK-NEXT:   8: b
// CHECK-NEXT:   9: [B10.8] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:  10: [B10.9].operator int
// CHECK-NEXT:  11: [B10.9]
// CHECK-NEXT:  12: [B10.11] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK-NEXT:  13: [B10.12] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: if [B10.13]
// CHECK-NEXT:   Preds (1): B11
// CHECK-NEXT:   Succs (2): B9 B5
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

// CHECK:      [B13 (ENTRY)]
// CHECK-NEXT:   Succs (1): B12
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B2 B10
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (2): B3 B7
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (2): B4 B6
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B3
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: 'a'
// CHECK-NEXT:   3: char c = 'a';
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: a
// CHECK-NEXT:   3: [B6.2] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   4: [B6.3] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   5: ![B6.4]
// CHECK-NEXT:   T: if [B6.5]
// CHECK-NEXT:   Preds (2): B8 B9
// CHECK-NEXT:   Succs (2): B5 B3
// CHECK:      [B7]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: 2
// CHECK-NEXT:   3: a
// CHECK-NEXT:   4: [B7.3] = [B7.2]
// CHECK-NEXT:   5: 3
// CHECK-NEXT:   6: b
// CHECK-NEXT:   7: [B7.6] = [B7.5]
// CHECK-NEXT:   8: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B8
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B8]
// CHECK-NEXT:   1: b
// CHECK-NEXT:   2: [B8.1] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   3: [B8.2] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   4: ![B8.3]
// CHECK-NEXT:   T: if [B9.4] && [B8.4]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B7 B6
// CHECK:      [B9]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: a
// CHECK-NEXT:   3: [B9.2] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   4: [B9.3] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: [B9.4] && ...
// CHECK-NEXT:   Preds (2): B11 B12
// CHECK-NEXT:   Succs (2): B8 B6
// CHECK:      [B10]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: 0
// CHECK-NEXT:   3: a
// CHECK-NEXT:   4: [B10.3] = [B10.2]
// CHECK-NEXT:   5: 1
// CHECK-NEXT:   6: b
// CHECK-NEXT:   7: [B10.6] = [B10.5]
// CHECK-NEXT:   8: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B11
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B11]
// CHECK-NEXT:   1: b
// CHECK-NEXT:   2: [B11.1] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   3: [B11.2] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: if [B12.4] && [B11.3]
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (2): B10 B9
// CHECK:      [B12]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: a
// CHECK-NEXT:   3: [B12.2] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   4: [B12.3] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: [B12.4] && ...
// CHECK-NEXT:   Preds (1): B13
// CHECK-NEXT:   Succs (2): B11 B9
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
// CHECK-NEXT:   1: [B4.4].~A() (Implicit destructor)
// CHECK-NEXT:   2: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   3: [B5.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeBegin(WhileStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B4.4].~A() (Implicit destructor)
// CHECK-NEXT:   6: CFGScopeEnd(WhileStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: a
// CHECK-NEXT:   2: [B4.1] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   3: [B4.2] (CXXConstructExpr, class A)
// CHECK-NEXT:   4: A b = a;
// CHECK-NEXT:   5: b
// CHECK-NEXT:   6: [B4.5] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   7: [B4.6].operator int
// CHECK-NEXT:   8: [B4.6]
// CHECK-NEXT:   9: [B4.8] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK:       10: [B4.9] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: while [B4.10]
// CHECK-NEXT:   Preds (2): B2 B5
// CHECK-NEXT:   Succs (2): B3 B1
// CHECK:      [B5]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
void test_while_implicit_scope() {
  A a;
  while (A b = a)
    A c;
}

// CHECK:      [B15 (ENTRY)]
// CHECK-NEXT:   Succs (1): B14
// CHECK:      [B1]
// CHECK-NEXT:   1: [B13.4].~A() (Implicit destructor)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A e;
// CHECK-NEXT:   4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   5: [B1.3].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B14.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (2): B11 B13
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   Preds (2): B3 B8
// CHECK-NEXT:   Succs (1): B13
// CHECK:      [B3]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A d;
// CHECK-NEXT:   3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   4: [B3.2].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B13.4].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B13.4].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B14.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B6]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B6.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B6.2]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B7]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   3: [B13.4].~A() (Implicit destructor)
// CHECK-NEXT:   T: continue;
// CHECK:        Preds (1): B9
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeEnd(ContinueStmt)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B9]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B9.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B9.2]
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (2): B7 B6
// CHECK:      [B10]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   T: break;
// CHECK:        Preds (1): B12
// CHECK-NEXT:   Succs (1): B11
// CHECK:      [B11]
// CHECK-NEXT:   1: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B12]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B12.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B12.5]
// CHECK-NEXT:   Preds (1): B13
// CHECK-NEXT:   Succs (2): B10 B9
// CHECK:      [B13]
// CHECK-NEXT:   1: a
// CHECK-NEXT:   2: [B13.1] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   3: [B13.2] (CXXConstructExpr, class A)
// CHECK-NEXT:   4: A b = a;
// CHECK-NEXT:   5: b
// CHECK-NEXT:   6: [B13.5] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   7: [B13.6].operator int
// CHECK-NEXT:   8: [B13.6]
// CHECK-NEXT:   9: [B13.8] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK-NEXT:  10: [B13.9] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: while [B13.10]
// CHECK-NEXT:   Preds (2): B2 B14
// CHECK-NEXT:   Succs (2): B12 B1
// CHECK:      [B14]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   Preds (1): B15
// CHECK-NEXT:   Succs (1): B13
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

// CHECK:      [B15 (ENTRY)]
// CHECK-NEXT:   Succs (1): B14
// CHECK:      [B1]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A d;
// CHECK-NEXT:   3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   4: [B1.2].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B14.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (2): B11 B2
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B2.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: do ... while [B2.2]
// CHECK-NEXT:   Preds (2): B3 B8
// CHECK-NEXT:   Succs (2): B13 B1
// CHECK:      [B3]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A c;
// CHECK-NEXT:   3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   4: [B3.2].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B14.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B6]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B6.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B6.2]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B7]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   T: continue;
// CHECK:        Preds (1): B9
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeEnd(ContinueStmt)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B9]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B9.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B9.2]
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (2): B7 B6
// CHECK:      [B10]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   T: break;
// CHECK:        Preds (1): B12
// CHECK-NEXT:   Succs (1): B11
// CHECK:      [B11]
// CHECK-NEXT:   1: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B12]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A b;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B12.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B12.5]
// CHECK-NEXT:   Preds (2): B13 B14
// CHECK-NEXT:   Succs (2): B10 B9
// CHECK:      [B13]
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B12
// CHECK:      [B14]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   Preds (1): B15
// CHECK-NEXT:   Succs (1): B12
// CHECK:      [B0 (EXIT)
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
// CHECK-NEXT:   2: [B4.4].~A() (Implicit destructor)
// CHECK-NEXT:   3: [B5.4].~A() (Implicit destructor)
// CHECK-NEXT:	 4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B3]
// CHECK-NEXT:	 1: CFGScopeBegin(ForStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B4.4].~A() (Implicit destructor)
// CHECK-NEXT:	 6: CFGScopeEnd(ForStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: a
// CHECK-NEXT:   2: [B4.1] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   3: [B4.2] (CXXConstructExpr, class A)
// CHECK-NEXT:   4: A b = a;
// CHECK-NEXT:   5: b
// CHECK-NEXT:   6: [B4.5] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   7: [B4.6].operator int
// CHECK-NEXT:   8: [B4.6]
// CHECK-NEXT:   9: [B4.8] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK:       10: [B4.9] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: for (...; [B4.10]; )
// CHECK-NEXT:   Preds (2): B2 B5
// CHECK-NEXT:   Succs (2): B3 B1
// CHECK:      [B5]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   3:  (CXXConstructExpr, class A)
// CHECK-NEXT:   4: A a;
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
void test_for_implicit_scope() {
  for (A a; A b = a; )
    A c;
}

// CHECK:      [B15 (ENTRY)]
// CHECK-NEXT:   Succs (1): B14
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   2: [B13.4].~A() (Implicit destructor)
// CHECK-NEXT:   3: [B14.6].~A() (Implicit destructor)
// CHECK-NEXT:   4:  (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A f;
// CHECK-NEXT:   6: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   7: [B1.5].~A() (Implicit destructor)
// CHECK-NEXT:   8: [B14.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (2): B11 B13
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   Preds (2): B3 B8
// CHECK-NEXT:   Succs (1): B13
// CHECK:      [B3]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A e;
// CHECK-NEXT:   3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   4: [B3.2].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B13.4].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B13.4].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B14.6].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B14.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B6]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B6.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B6.2]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B7]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   T: continue;
// CHECK:        Preds (1): B9
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeEnd(ContinueStmt)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B9]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B9.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B9.2]
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (2): B7 B6
// CHECK:      [B10]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B12.3].~A() (Implicit destructor)
// CHECK-NEXT:   T: break;
// CHECK:        Preds (1): B12
// CHECK-NEXT:   Succs (1): B11
// CHECK:      [B11]
// CHECK-NEXT:   1: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B12]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A d;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B12.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B12.5]
// CHECK-NEXT:   Preds (1): B13
// CHECK-NEXT:   Succs (2): B10 B9
// CHECK:      [B13]
// CHECK-NEXT:   1: b
// CHECK-NEXT:   2: [B13.1] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   3: [B13.2] (CXXConstructExpr, class A)
// CHECK-NEXT:   4: A c = b;
// CHECK-NEXT:   5: c
// CHECK-NEXT:   6: [B13.5] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   7: [B13.6].operator int
// CHECK-NEXT:   8: [B13.6]
// CHECK-NEXT:   9: [B13.8] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK-NEXT:  10: [B13.9] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: for (...; [B13.10]; )
// CHECK-NEXT:   Preds (2): B2 B14
// CHECK-NEXT:   Succs (2): B12 B1
// CHECK:      [B14]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   5:  (CXXConstructExpr, class A)
// CHECK-NEXT:   6: A b;
// CHECK-NEXT:   Preds (1): B15
// CHECK-NEXT:   Succs (1): B13
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

// CHECK:      [B10 (ENTRY)]
// CHECK-NEXT:   Succs (1): B9
// CHECK:      [B1]
// CHECK-NEXT:  l1:
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A c;
// CHECK-NEXT:   3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   4: [B1.2].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B8.2].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (2): B2 B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A b;
// CHECK-NEXT:   3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   4: [B2.2].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B8.5].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (2): B3 B5
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B8.5].~A() (Implicit destructor)
// CHECK-NEXT:   T: goto l1;
// CHECK:        Preds (1): B5
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B5]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B5.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B5.2]
// CHECK-NEXT:   Preds (2): B6 B8
// CHECK-NEXT:   Succs (2): B4 B2
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B7]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B8.5].~A() (Implicit destructor)
// CHECK-NEXT:   3: [B8.2].~A() (Implicit destructor)
// CHECK-NEXT:   T: goto l0;
// CHECK:        Preds (1): B8
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B8]
// CHECK-NEXT:  l0:
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A b;
// CHECK-NEXT:   3: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   4:  (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A a;
// CHECK-NEXT:   6: UV
// CHECK-NEXT:   7: [B8.6] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B8.7]
// CHECK-NEXT:   Preds (2): B9 B7
// CHECK-NEXT:   Succs (2): B7 B5
// CHECK:      [B9]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B0 (EXIT)]
void test_goto() {
  A a;
l0:
  A b;
  { A a;
    if (UV) goto l0;
    if (UV) goto l1;
    A b;
  }
l1:
  A c;
}

// CHECK:      [B11 (ENTRY)]
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   2: 1
// CHECK-NEXT:   3: int k = 1;
// CHECK-NEXT:   4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: '1'
// CHECK-NEXT:   3: char c = '1';
// CHECK-NEXT:   4: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   5: getX
// CHECK-NEXT:   6: [B2.5] (ImplicitCastExpr, FunctionToPointerDecay, int (*)(void))
// CHECK-NEXT:   7: [B2.6]()
// CHECK-NEXT:   8: int i = getX();
// CHECK-NEXT:   9: i
// CHECK-NEXT:  10: [B2.9] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   T: switch [B2.10]
// CHECK-NEXT:   Preds (1): B11
// CHECK-NEXT:   Succs (1): B3
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   T: switch [B2.10]
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (5): B6 B7 B9 B10 B5
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (3): B5 B8 B9
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B5]
// CHECK-NEXT:  default:
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: 0
// CHECK-NEXT:   3: int a = 0;
// CHECK-NEXT:   4: i
// CHECK-NEXT:   5: ++[B5.4]
// CHECK-NEXT:   6: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B6 B3
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B6]
// CHECK-NEXT:  case 3:
// CHECK-NEXT:   1: '2'
// CHECK-NEXT:   2: c
// CHECK-NEXT:   3: [B6.2] = [B6.1]
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B7]
// CHECK-NEXT:  case 2:
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: '2'
// CHECK-NEXT:   3: c
// CHECK-NEXT:   4: [B7.3] = [B7.2]
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B9]
// CHECK-NEXT:  case 1:
// CHECK-NEXT:   1: '3'
// CHECK-NEXT:   2: c
// CHECK-NEXT:   3: [B9.2] = [B9.1]
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (2): B3 B10
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B10]
// CHECK-NEXT:  case 0:
// CHECK-NEXT:   1: '2'
// CHECK-NEXT:   2: c
// CHECK-NEXT:   3: [B10.2] = [B10.1]
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B9
// CHECK:      [B0 (EXIT)]
void test_switch_with_compound_with_default() {
  char c = '1';
  switch (int i = getX()) {
  case 0:
    c = '2';
  case 1:
    c = '3';
    break;
  case 2: {
    c = '2';
    break;
  }
  case 3:
    c = '2';
  default: {
    int a = 0;
    ++i;
  }
  }
  int k = 1;
}

// CHECK:      [B9 (ENTRY)]
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   2: 3
// CHECK-NEXT:   3: int k = 3;
// CHECK-NEXT:   4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: '1'
// CHECK-NEXT:   3: char c = '1';
// CHECK-NEXT:   4: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   5: getX
// CHECK-NEXT:   6: [B2.5] (ImplicitCastExpr, FunctionToPointerDecay, int (*)(void))
// CHECK-NEXT:   7: [B2.6]()
// CHECK-NEXT:   8: int i = getX();
// CHECK-NEXT:   9: i
// CHECK-NEXT:  10: [B2.9] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   T: switch [B2.10]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (1): B3
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   T: switch [B2.10]
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (4): B5 B6 B8 B4
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (3): B5 B7 B3
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B5]
// CHECK-NEXT:  case 2:
// CHECK-NEXT:   1: '3'
// CHECK-NEXT:   2: c
// CHECK-NEXT:   3: [B5.2] = [B5.1]
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B6]
// CHECK-NEXT:  case 1:
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: '1'
// CHECK-NEXT:   3: c
// CHECK-NEXT:   4: [B6.3] = [B6.2]
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (2): B3 B8
// CHECK-NEXT:   Succs (1): B7
// CHECK:      [B7]
// CHECK-NEXT:   1: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B8]
// CHECK-NEXT:  case 0:
// CHECK-NEXT:   1: '2'
// CHECK-NEXT:   2: c
// CHECK-NEXT:   3: [B8.2] = [B8.1]
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B6
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
int test_switch_with_compound_without_default() {
  char c = '1';
  switch (int i = getX()) {
  case 0:
    c = '2';
  case 1: {
    c = '1';
    break;
  }
  case 2:
    c = '3';
    break;
  }
  int k = 3;
}

// CHECK:      [B5 (ENTRY)]
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   2: 1
// CHECK-NEXT:   3: int k = 1;
// CHECK-NEXT:   4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: '1'
// CHECK-NEXT:   3: char s = '1';
// CHECK-NEXT:   4: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   5: getX
// CHECK-NEXT:   6: [B2.5] (ImplicitCastExpr, FunctionToPointerDecay, int (*)(void))
// CHECK-NEXT:   7: [B2.6]()
// CHECK-NEXT:   8: int i = getX();
// CHECK-NEXT:   9: i
// CHECK-NEXT:  10: [B2.9] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   T: switch [B2.10]
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B3]
// CHECK-NEXT:  default:
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: 0
// CHECK-NEXT:   3: int a = 0;
// CHECK-NEXT:   4: i
// CHECK-NEXT:   5: ++[B3.4]
// CHECK-NEXT:   6: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B4 B2
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B4]
// CHECK-NEXT:  case 0:
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B3
// CHECK: [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
void test_without_compound() {
  char s = '1';
  switch (int i = getX())
  case 0:
  default: {
    int a = 0;
    ++i;
  }
  int k = 1;
}

// CHECK:      [B8 (ENTRY)]
// CHECK-NEXT:   Succs (1): B7
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(DeclStmt)
// CHECK-NEXT:   2: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   3: int unused2;
// CHECK-NEXT:   4: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   5: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B5 B6
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: i
// CHECK-NEXT:   2: ++[B2.1]
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B6
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   3: int unused1;
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B6]
// CHECK-NEXT:   1: i
// CHECK-NEXT:   2: [B6.1] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   3: 3
// CHECK-NEXT:   4: [B6.2] < [B6.3]
// CHECK-NEXT:   T: for (...; [B6.4]; ...)
// CHECK-NEXT:   Preds (2): B2 B7
// CHECK-NEXT:   Succs (2): B4 B1
// CHECK:      [B7]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: CFGScopeBegin(DeclStmt)
// CHECK-NEXT:   3: 0
// CHECK-NEXT:   4: int i = 0;
// CHECK-NEXT:   Preds (1): B8
// CHECK-NEXT:   Succs (1): B6
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

// CHECK:      [B16 (ENTRY)]
// CHECK-NEXT:   Succs (1): B15
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(BinaryOperator)
// CHECK-NEXT:   2: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B5 B14
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: i
// CHECK-NEXT:   2: ++[B2.1]
// CHECK-NEXT:   Preds (2): B3 B11
// CHECK-NEXT:   Succs (1): B14
// CHECK:      [B3]
// CHECK-NEXT:   1: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: 5
// CHECK-NEXT:   3: int z = 5;
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (1): B8
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: x
// CHECK-NEXT:   3: [B6.2] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   T: switch [B6.3]
// CHECK-NEXT:   Preds (1): B14
// CHECK-NEXT:   Succs (1): B7
// CHECK:      [B7]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   T: switch [B6.3]
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (4): B10 B12 B13 B9
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B9 B12
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B9]
// CHECK-NEXT:  default:
// CHECK-NEXT:   1: 3
// CHECK-NEXT:   2: y
// CHECK-NEXT:   3: [B9.2] = [B9.1]
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B10]
// CHECK-NEXT:  case 2:
// CHECK-NEXT:   1: 4
// CHECK-NEXT:   2: y
// CHECK-NEXT:   3: [B10.2] = [B10.1]
// CHECK-NEXT:   T: continue;
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B11
// CHECK:      [B11]
// CHECK-NEXT:   1: CFGScopeEnd(ContinueStmt)
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B12]
// CHECK-NEXT:  case 1:
// CHECK-NEXT:   1: 2
// CHECK-NEXT:   2: y
// CHECK-NEXT:   3: [B12.2] = [B12.1]
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (2): B7 B13
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B13]
// CHECK-NEXT:  case 0:
// CHECK-NEXT:   1: 1
// CHECK-NEXT:   2: y
// CHECK-NEXT:   3: [B13.2] = [B13.1]
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B12
// CHECK:      [B14]
// CHECK-NEXT:   1: i
// CHECK-NEXT:   2: [B14.1] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   3: 1000
// CHECK-NEXT:   4: [B14.2] < [B14.3]
// CHECK-NEXT:   T: for (...; [B14.4]; ...)
// CHECK-NEXT:   Preds (2): B2 B15
// CHECK-NEXT:   Succs (2): B6 B1
// CHECK:      [B15]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: int i;
// CHECK-NEXT:   3: int x;
// CHECK-NEXT:   4: int y;
// CHECK-NEXT:   5: CFGScopeBegin(BinaryOperator)
// CHECK-NEXT:   6: 0
// CHECK-NEXT:   7: i
// CHECK-NEXT:   8: [B15.7] = [B15.6]
// CHECK-NEXT:   Preds (1): B16
// CHECK-NEXT:   Succs (1): B14
// CHECK:      [B0 (EXIT)]
void test_for_switch_in_for() {
  int i, x, y;
  for (i = 0; i < 1000; ++i) {
    switch (x) {
      case 0:
        y = 1;
      case 1:
        y = 2;
        break; // break from switch
      case 2:
        y = 4;
        continue; // continue in loop
      default:
        y = 3;
    }
   {
     int z = 5;
     break; // break from loop
   }
  }
}

int vfork();
void execl(char *, char *, int);
int _exit(int);
// CHECK:      [B20 (ENTRY)]
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1: CFGScopeBegin(WhileStmt)
// CHECK-NEXT:   2: CFGScopeEnd(WhileStmt)
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B3
// CHECK:      [B3]
// CHECK-NEXT:   1: 1
// CHECK-NEXT:   2: [B3.1] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: while [B3.2]
// CHECK-NEXT:   Preds (2): B2 B6
// CHECK-NEXT:   Succs (2): B2 NULL
// CHECK:      [B4]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: x
// CHECK-NEXT:   3: [B4.2] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   T: switch [B4.3]
// CHECK-NEXT:   Preds (1): B20
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B5]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   T: switch [B4.3]
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (4): B12 B15 B19 B6
// CHECK:      [B6]
// CHECK-NEXT:   1: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (4): B8 B14 B19 B5
// CHECK-NEXT:   Succs (1): B3
// CHECK:      [B7]
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (2): B9 B12
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B8]
// CHECK-NEXT:   1: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B6
// CHECK:      [B9]
// CHECK-NEXT:   1: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Succs (1): B7
// CHECK:      [B10]
// CHECK-NEXT:   1: CFGScopeBegin(WhileStmt)
// CHECK-NEXT:   2: CFGScopeEnd(WhileStmt)
// CHECK-NEXT:   Preds (1): B11
// CHECK-NEXT:   Succs (1): B11
// CHECK:      [B11]
// CHECK-NEXT:   1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: 1
// CHECK-NEXT:   3: [B11.2] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: while [B11.3]
// CHECK-NEXT:   Preds (2): B10 B12
// CHECK-NEXT:   Succs (2): B10 NULL
// CHECK:      [B12]
// CHECK-NEXT:  case 2:
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: int pid;
// CHECK-NEXT:   3: vfork
// CHECK-NEXT:   4: [B12.3] (ImplicitCastExpr, FunctionToPointerDecay, int (*)(void))
// CHECK-NEXT:   5: [B12.4]()
// CHECK-NEXT:   6: pid
// CHECK-NEXT:   7: [B12.6] = [B12.5]
// CHECK-NEXT:   8: ([B12.7]) (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:   9: 0
// CHECK-NEXT:  10: [B12.8] == [B12.9]
// CHECK-NEXT:   T: if [B12.10]
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (2): B11 B7
// CHECK:      [B13]
// CHECK-NEXT:   1: 3
// CHECK-NEXT:   2: x
// CHECK-NEXT:   3: [B13.2] = [B13.1]
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (1): B17
// CHECK-NEXT:   Succs (1): B14
// CHECK:      [B14]
// CHECK-NEXT:   1: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   Preds (1): B13
// CHECK-NEXT:   Succs (1): B6
// CHECK:      [B15]
// CHECK-NEXT:  case 1:
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: char args[2];
// CHECK-NEXT:   3: vfork
// CHECK-NEXT:   4: [B15.3] (ImplicitCastExpr, FunctionToPointerDecay, int (*)(void))
// CHECK-NEXT:   5: [B15.4]()
// CHECK-NEXT:   T: switch [B15.5]
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B16
// CHECK:      [B16]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   T: switch [B15.5]
// CHECK-NEXT:   Preds (1): B15
// CHECK-NEXT:   Succs (2): B18 B17
// CHECK:      [B17]
// CHECK-NEXT:   1: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B18 B16
// CHECK-NEXT:   Succs (1): B13
// CHECK:      [B18]
// CHECK-NEXT:  case 0:
// CHECK-NEXT:   1: 0
// CHECK-NEXT:   2: [B18.1] (ImplicitCastExpr, IntegralCast, char)
// CHECK-NEXT:   3: args
// CHECK-NEXT:   4: [B18.3] (ImplicitCastExpr, ArrayToPointerDecay, char *)
// CHECK-NEXT:   5: 0
// CHECK:        7: [B18.6] = [B18.2]
// CHECK-NEXT:   8: _exit
// CHECK-NEXT:   9: [B18.8] (ImplicitCastExpr, FunctionToPointerDecay, int (*)(int))
// CHECK-NEXT:  10: 1
// CHECK-NEXT:  11: [B18.9]([B18.10])
// CHECK-NEXT:   Preds (1): B16
// CHECK-NEXT:   Succs (1): B17
// CHECK:      [B19]
// CHECK-NEXT:  case 0:
// CHECK-NEXT:   1: vfork
// CHECK-NEXT:   2: [B19.1] (ImplicitCastExpr, FunctionToPointerDecay, int (*)(void))
// CHECK-NEXT:   3: [B19.2]()
// CHECK-NEXT:   4: 0
// CHECK-NEXT:   5: x
// CHECK-NEXT:   6: [B19.5] = [B19.4]
// CHECK-NEXT:   T: break;
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B6
// CHECK:      [B0 (EXIT)]
void test_nested_switches(int x) {
  switch (x) {
  case 0:
    vfork();
    x = 0;
    break;

  case 1: {
    char args[2];
    switch (vfork()) {
    case 0:
      args[0] = 0;
      _exit(1);
    }
    x = 3;
    break;
  }

  case 2: {
    int pid;
    if ((pid = vfork()) == 0)
      while (1)
        ;
    break;
  }
  }
  while (1)
    ;
}
