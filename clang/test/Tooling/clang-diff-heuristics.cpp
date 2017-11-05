// RUN: %clang_cc1 -E %s > %t.src.cpp
// RUN: %clang_cc1 -E %s > %t.dst.cpp -DDEST
// RUN: clang-diff -dump-matches -s=0 %t.src.cpp %t.dst.cpp -- | FileCheck %s
//
// Test the heuristics, with maxsize set to 0, so that the optimal matching will never be applied.

#ifndef DEST

void f1() {;}

void f2(int) {;}

class C3 { C3(); };

#else

// same parents, same value
// CHECK: Match FunctionDecl(1) to FunctionDecl(1)
// CHECK: Match CompoundStmt
void f1() {}

// same parents, same identifier
// CHECK: Match FunctionDecl(6) to FunctionDecl(5)
// CHECK: Match CompoundStmt
void f2() {}

// same parents, same identifier
// CHECK: Match CXXConstructorDecl(14) to CXXConstructorDecl(10)
class C3 { C3(int); };

#endif
