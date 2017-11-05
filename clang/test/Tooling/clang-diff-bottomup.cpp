// RUN: %clang_cc1 -E %s > %t.src.cpp
// RUN: %clang_cc1 -E %s > %t.dst.cpp -DDEST
// RUN: clang-diff -dump-matches -s=0 -stop-diff-after=bottomup %t.src.cpp %t.dst.cpp -- | FileCheck %s
//
// Test the bottom-up matching, with maxsize set to 0, so that the optimal matching will never be applied.

#ifndef DEST

void f1(int) { ; {{;}} }
void f2(int) { ;; {{;}} }

#else

// Jaccard similarity threshold is 0.5.

void f1() {
// CompoundStmt: 3 matched descendants, subtree sizes 4 and 5
// Jaccard similarity = 3 / (4 + 5 - 3) = 3 / 6 >= 0.5
// CHECK: Match CompoundStmt(6) to CompoundStmt(4)
// CHECK-NEXT: Move CompoundStmt
// CHECK-NEXT: Match CompoundStmt
// CHECK-NEXT: Match CompoundStmt
// CHECK-NEXT: Match NullStmt
  {{;}} ;;
}

void f2() {
// CompoundStmt: 3 matched descendants, subtree sizes 4 and 5
// Jaccard similarity = 3 / (5 + 6 - 3) = 3 / 8 < 0.5
// CHECK-NOT: Match CompoundStmt(16)
// CHECK: Match CompoundStmt(19) to CompoundStmt(14)
// CHECK-NEXT: Move CompoundStmt
// CHECK-NEXT: Match CompoundStmt
// CHECK-NEXT: Match NullStmt
// CHECK-NOT: Match NullStmt({{.*}})
  {{;}} ;;;
}

#endif
