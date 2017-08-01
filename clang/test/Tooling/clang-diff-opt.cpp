// RUN: %clang_cc1 -E %s > %t.src.cpp
// RUN: %clang_cc1 -E %s > %t.dst.cpp -DDEST
// RUN: clang-diff -m -no-compilation-database %t.src.cpp %t.dst.cpp | FileCheck %s
//
// Test the behaviour of the matching according to the optimal tree edit
// distance, implemented with Zhang and Shasha's algorithm.

#ifndef DEST

void f1() { {;} {{;}} }

#else

void f1() {
// Jaccard similarity = 3 / (5 + 4 - 3) = 3 / 6 >= 0.5
// The optimal matching algorithm should move the ; into the outer block
// CHECK: Match CompoundStmt(2) to CompoundStmt(2)
// CHECK-NOT: Match CompoundStmt(3)
// CHECK: Match NullStmt(4) to NullStmt(3)
  ; {{;}}
}
 
#endif
