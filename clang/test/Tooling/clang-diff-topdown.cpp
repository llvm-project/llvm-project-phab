// RUN: %clang_cc1 -E %s > %t.src.cpp
// RUN: %clang_cc1 -E %s > %t.dst.cpp -DDEST
// RUN: clang-diff -dump-matches -stop-diff-after=topdown %t.src.cpp %t.dst.cpp -- -std=c++11 | FileCheck %s
//
// Test the top-down matching of identical subtrees only.

#ifndef DEST

void f1()
{
  // Match some subtree of height greater than 2.
  // CHECK: Match CompoundStmt(5) to CompoundStmt(5)
  // CHECK-NEXT: Move CompoundStmt
  // CHECK-NEXT: Match CompoundStmt
  // CHECK-NEXT: Match NullStmt({{.*}}) to NullStmt({{.*}})
  {{;}}

  // Don't match subtrees that are smaller.
  // CHECK-NOT: Match CompoundStmt(6)
  // CHECK-NOT: Match NullStmt(7)
  {;}

  // Greedy approach - use the first matching subtree when there are multiple
  // identical subtrees.
  // CHECK: Match CompoundStmt(10) to CompoundStmt(10)
  // CHECK-NEXT: Move CompoundStmt
  // CHECK-NEXT: Match CompoundStmt({{.*}}) to CompoundStmt({{.*}})
  // CHECK-NEXT: Match NullStmt({{.*}}) to NullStmt({{.*}})
  {{;;}}
}

int x;

namespace src {
  int x;
  int x1 = x + 1;
  int x2 = ::x + 1;
}

#else


void f1() {

  {{;}}

  {;}

  {{;;}}
  // CHECK-NOT: Match {{.*}} to CompoundStmt(15)
  // CHECK-NOT: Match {{.*}} to CompoundStmt(16)
  // CHECK-NOT: Match {{.*}} to NullStmt(17)
  {{;;}}

  // CHECK-NOT: Match {{.*}} to NullStmt(18)
  ;
}

int x;

namespace dst {
  int x;
  // CHECK: Match DeclRefExpr(22) to DeclRefExpr(27)
  int x1 = x + 1;
  // CHECK: Match DeclRefExpr
  int x2 = ::x + 1;
}

#endif
