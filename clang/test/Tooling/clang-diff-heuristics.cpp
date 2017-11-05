// RUN: %clang_cc1 -E %s > %t.src.cpp
// RUN: %clang_cc1 -E %s > %t.dst.cpp -DDEST
// RUN: clang-diff -dump-matches -s=0 %t.src.cpp %t.dst.cpp -- | FileCheck %s
//
// Test the heuristics, with maxsize set to 0, so that the optimal matching will never be applied.

#ifndef DEST

void f1() {;}

void f2(int) {;}

#else

// same parents, same value
// CHECK: Match FunctionDecl: f1(void ())(1) to FunctionDecl: f1(void ())(1)
// CHECK: Match CompoundStmt
void f1() {}

// same parents, same identifier
// CHECK: Match FunctionDecl: f2(void (int))(4) to FunctionDecl: f2(void ())(3)
// CHECK: Match CompoundStmt
void f2() {}

#endif
