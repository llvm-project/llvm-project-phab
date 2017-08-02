// RUN: %clang_cc1 -E %s > %t.src.cpp
// RUN: %clang_cc1 -E %s > %t.dst.cpp -DDEST
// RUN: clang-diff -m -no-compilation-database %t.src.cpp %t.dst.cpp | FileCheck %s
//
// Test the behaviour of the matching according to the optimal tree edit
// distance, implemented with Zhang and Shasha's algorithm.

#ifndef DEST

void f1() { {;} {{;}} }

class A { int x; void f() { int a1 = x; } };

#else

void f1() {
// Jaccard similarity = 3 / (5 + 4 - 3) = 3 / 6 >= 0.5
// The optimal matching algorithm should move the ; into the outer block
// CHECK: Match CompoundStmt(2) to CompoundStmt(2)
// CHECK-NOT: Match CompoundStmt(3)
// CHECK: Match NullStmt(4) to NullStmt(3)
  ; {{;}}
}

class B {
  // Only the class name changed; it is not included in the field value,
  // therefore there is no update.
  // CHECK: Match FieldDecl: :x(int)(9) to FieldDecl: :x(int)(8)
  // CHECK-NOT: Update FieldDecl: :x(int)(9)
  int x;
  void f() {
    // CHECK: Match MemberExpr: :x(14) to MemberExpr: :x(13)
    // CHECK-NOT: Update MemberExpr: :x(14)
    int b2 = B::x;
  }
};
 
#endif
