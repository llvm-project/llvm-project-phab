// RUN: %clang_cc1 -fcxx-exceptions -fexceptions -analyze -analyzer-checker=debug.DumpCFG -analyzer-config cfg-scopes=true %s > %t 2>&1
// RUN: FileCheck --input-file=%t %s

class A {
public:
// CHECK:      [B2 (ENTRY)]
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
  A() {}

// CHECK:      [B2 (ENTRY)]
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B1]
// CHECK-NEXT:   1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B2
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
// CHECK-NEXT:   10: [B1.9]
// CHECK:       11: const A &c = A();
// CHECK:       12: [B1.11].~A() (Implicit destructor)
// CHECK:       13: [B1.3].~A() (Implicit destructor)
// CHECK:       14: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
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
// CHECK-NEXT:   10: [B1.6].~A() (Implicit destructor)
// CHECK-NEXT:   11: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   12:  (CXXConstructExpr, class A)
// CHECK:       13: A b;
// CHECK:       14: [B1.13].~A() (Implicit destructor)
// CHECK:       15: [B1.3].~A() (Implicit destructor)
// CHECK:	16: CFGScopeEnd(CompoundStmt)
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

// CHECK:      [B4 (ENTRY)]
// CHECK-NEXT:   Succs (1): B3
// CHECK:      [B1]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A c;
// CHECK-NEXT:   3: [B1.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B3.5].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:	 6: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B3.5].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:	 5: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B3]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
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
// CHECK-NEXT:   1: [B4.7].~A() (Implicit destructor)
// CHECK-NEXT:   2: [B4.3].~A() (Implicit destructor)
// CHECK-NEXT:	 3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B2 B3
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: [B2.3].~A() (Implicit destructor)
// CHECK-NEXT:	 5: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B3]
// CHECK-NEXT:	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: CFGScopeEnd(IfStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B4]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4: a
// CHECK-NEXT:   5: [B4.4] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   6: [B4.5] (CXXConstructExpr, class A)
// CHECK-NEXT:   7: A b = a;
// CHECK-NEXT:   8: b
// CHECK-NEXT:   9: [B4.8] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   10: [B4.9].operator int
// CHECK:       11: [B4.9]
// CHECK:       12: [B4.11] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK:       13: [B4.12] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: if [B4.13]
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (2): B3 B2
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
void test_if_implicit_scope() {
  A a;
  if (A b = a)
    A c;
  else A c;
}

// CHECK:      [B9 (ENTRY)]
// CHECK-NEXT:   Succs (1): B8
// CHECK:      [B1]
// CHECK-NEXT:   1: [B8.7].~A() (Implicit destructor)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A e;
// CHECK-NEXT:   4: [B1.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B8.3].~A() (Implicit destructor)
// CHECK-NEXT:	 6: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B2 B5
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A d;
// CHECK-NEXT:   3: [B2.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B4.3].~A() (Implicit destructor)
// CHECK-NEXT:	 5: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B3]
// CHECK-NEXT:	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B4.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B8.7].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B8.3].~A() (Implicit destructor)
// CHECK-NEXT:	 6: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B4]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
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
// CHECK-NEXT:	 5: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B6]
// CHECK-NEXT: 	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B7.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B8.7].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B8.3].~A() (Implicit destructor)
// CHECK-NEXT:	 6: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B7]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B7.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B7.5]
// CHECK-NEXT:   Preds (1): B8
// CHECK-NEXT:   Succs (2): B6 B5
// CHECK:      [B8]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4: a
// CHECK-NEXT:   5: [B8.4] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   6: [B8.5] (CXXConstructExpr, class A)
// CHECK-NEXT:   7: A b = a;
// CHECK-NEXT:   8: b
// CHECK-NEXT:   9: [B8.8] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   10: [B8.9].operator int
// CHECK:       11: [B8.9]
// CHECK:       12: [B8.11] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK:       13: [B8.12] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: if [B8.13]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B7 B4
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (3): B1 B3 B6
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

// CHECK:      [B6 (ENTRY)]
// CHECK-NEXT:   Succs (1): B5
// CHECK:      [B1]
// CHECK-NEXT:   1: [B4.4].~A() (Implicit destructor)
// CHECK-NEXT:   2: [B5.3].~A() (Implicit destructor)
// CHECK-NEXT:	 3: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B4
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   Preds (1): B3
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B3]
// CHECK-NEXT:	 1: CFGScopeBegin(WhileStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: [B3.3].~A() (Implicit destructor)
// CHECK-NEXT:	 5: CFGScopeEnd(WhileStmt)
// CHECK-NEXT:   6: [B4.4].~A() (Implicit destructor)
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

// CHECK:      [B12 (ENTRY)]
// CHECK-NEXT:   Succs (1): B11
// CHECK:      [B1]
// CHECK-NEXT:   1: [B10.4].~A() (Implicit destructor)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A e;
// CHECK-NEXT:   4: [B1.3].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B11.3].~A() (Implicit destructor)
// CHECK-NEXT:	 6: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B8 B10
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   Preds (2): B3 B6
// CHECK-NEXT:   Succs (1): B10
// CHECK:      [B3]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A d;
// CHECK-NEXT:   3: [B3.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:	 5: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   6: [B10.4].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B10.4].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B11.3].~A() (Implicit destructor)
// CHECK-NEXT:	 6: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B5]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B5.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B5.2]
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B6]
// CHECK-NEXT: 	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   3: [B10.4].~A() (Implicit destructor)
// CHECK-NEXT:	 4: CFGScopeEnd(ContinueStmt)
// CHECK-NEXT:   T: continue;
// CHECK:        Preds (1): B7
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B7]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B7.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B7.2]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B6 B5
// CHECK:      [B8]
// CHECK-NEXT:	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:	 3: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   T: break;
// CHECK:        Preds (1): B9
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B9]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A c;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B9.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B9.5]
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (2): B8 B7
// CHECK:      [B10]
// CHECK-NEXT:   1: a
// CHECK-NEXT:   2: [B10.1] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   3: [B10.2] (CXXConstructExpr, class A)
// CHECK-NEXT:   4: A b = a;
// CHECK-NEXT:   5: b
// CHECK-NEXT:   6: [B10.5] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   7: [B10.6].operator int
// CHECK-NEXT:   8: [B10.6]
// CHECK-NEXT:   9: [B10.8] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK:       10: [B10.9] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: while [B10.10]
// CHECK-NEXT:   Preds (2): B2 B11
// CHECK-NEXT:   Succs (2): B9 B1
// CHECK:      [B11]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (1): B10
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (2): B1 B4
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
// CHECK-NEXT: 	 5: CFGScopeEnd(CompoundStmt)
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
// CHECK-NEXT:	 5: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT:	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B11.3].~A() (Implicit destructor)
// CHECK-NEXT:	 5: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B5]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B5.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B5.2]
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B6]
// CHECK-NEXT:	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:	 3: CFGScopeEnd(ContinueStmt)
// CHECK-NEXT:   T: continue;
// CHECK:        Preds (1): B7
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B7]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B7.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B7.2]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B6 B5
// CHECK:      [B8]
// CHECK-NEXT:	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:	 3: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   T: break;
// CHECK:        Preds (1): B9
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B9]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
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
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
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
// CHECK-NEXT:   1: [B4.4].~A() (Implicit destructor)
// CHECK-NEXT:   2: [B5.3].~A() (Implicit destructor)
// CHECK-NEXT:	 3: CFGScopeEnd(CompoundStmt)
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
// CHECK-NEXT:	 5: CFGScopeEnd(ForStmt)
// CHECK-NEXT:   6: [B4.4].~A() (Implicit destructor)
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
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   Preds (1): B6
// CHECK-NEXT:   Succs (1): B4
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
void test_for_implicit_scope() {
  for (A a; A b = a; )
    A c;
}

// CHECK:      [B12 (ENTRY)]
// CHECK-NEXT:   Succs (1): B11
// CHECK:      [B1]
// CHECK-NEXT:   1: [B10.4].~A() (Implicit destructor)
// CHECK-NEXT:   2: [B11.5].~A() (Implicit destructor)
// CHECK-NEXT:   3:  (CXXConstructExpr, class A)
// CHECK-NEXT:   4: A f;
// CHECK-NEXT:   5: [B1.4].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B11.3].~A() (Implicit destructor)
// CHECK-NEXT:	 7: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   Preds (2): B8 B10
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B2]
// CHECK-NEXT:   Preds (2): B3 B6
// CHECK-NEXT:   Succs (1): B10
// CHECK:      [B3]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: A e;
// CHECK-NEXT:   3: [B3.2].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:	 5: CFGScopeEnd(CompoundStmt)
// CHECK-NEXT:   6: [B10.4].~A() (Implicit destructor)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B4]
// CHECK-NEXT: 	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: return;
// CHECK-NEXT:   3: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:   4: [B10.4].~A() (Implicit destructor)
// CHECK-NEXT:   5: [B11.5].~A() (Implicit destructor)
// CHECK-NEXT:   6: [B11.3].~A() (Implicit destructor)
// CHECK-NEXT:	 7: CFGScopeEnd(ReturnStmt)
// CHECK-NEXT:   Preds (1): B5
// CHECK-NEXT:   Succs (1): B0
// CHECK:      [B5]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B5.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B5.2]
// CHECK-NEXT:   Preds (1): B7
// CHECK-NEXT:   Succs (2): B4 B3
// CHECK:      [B6]
// CHECK-NEXT:	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:	 3: CFGScopeEnd(ContinueStmt)
// CHECK-NEXT:   T: continue;
// CHECK:        Preds (1): B7
// CHECK-NEXT:   Succs (1): B2
// CHECK:      [B7]
// CHECK-NEXT:   1: UV
// CHECK-NEXT:   2: [B7.1] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B7.2]
// CHECK-NEXT:   Preds (1): B9
// CHECK-NEXT:   Succs (2): B6 B5
// CHECK:      [B8]
// CHECK-NEXT:	 1: CFGScopeBegin(IfStmt)
// CHECK-NEXT:   2: [B9.3].~A() (Implicit destructor)
// CHECK-NEXT:	 3: CFGScopeEnd(BreakStmt)
// CHECK-NEXT:   T: break;
// CHECK:        Preds (1): B9
// CHECK-NEXT:   Succs (1): B1
// CHECK:      [B9]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A d;
// CHECK-NEXT:   4: UV
// CHECK-NEXT:   5: [B9.4] (ImplicitCastExpr, LValueToRValue, _Bool)
// CHECK-NEXT:   T: if [B9.5]
// CHECK-NEXT:   Preds (1): B10
// CHECK-NEXT:   Succs (2): B8 B7
// CHECK:      [B10]
// CHECK-NEXT:   1: b
// CHECK-NEXT:   2: [B10.1] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   3: [B10.2] (CXXConstructExpr, class A)
// CHECK-NEXT:   4: A c = b;
// CHECK-NEXT:   5: c
// CHECK-NEXT:   6: [B10.5] (ImplicitCastExpr, NoOp, const class A)
// CHECK-NEXT:   7: [B10.6].operator int
// CHECK-NEXT:   8: [B10.6]
// CHECK-NEXT:   9: [B10.8] (ImplicitCastExpr, UserDefinedConversion, int)
// CHECK:       10: [B10.9] (ImplicitCastExpr, IntegralToBoolean, _Bool)
// CHECK-NEXT:   T: for (...; [B10.10]; )
// CHECK-NEXT:   Preds (2): B2 B11
// CHECK-NEXT:   Succs (2): B9 B1
// CHECK:      [B11]
// CHECK-NEXT:	 1: CFGScopeBegin(CompoundStmt)
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: A a;
// CHECK-NEXT:   4:  (CXXConstructExpr, class A)
// CHECK-NEXT:   5: A b;
// CHECK-NEXT:   Preds (1): B12
// CHECK-NEXT:   Succs (1): B10
// CHECK:      [B0 (EXIT)]
// CHECK-NEXT:   Preds (2): B1 B4
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
