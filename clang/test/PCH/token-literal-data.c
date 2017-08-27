// RUN: rm -fr %s_tmp.h.pch
// RUN: echo '#define MACRO 42' > %s_tmp.h
// RUN: %clang_cc1 -emit-pch -o %s_tmp.h.pch %s_tmp.h
// RUN: rm %s_tmp.h
// RUN: %clang_cc1 -fno-validate-pch -include-pch %s_tmp.h.pch -fsyntax-only -verify %s

// expected-no-diagnostics

int f(int x) { return x; }
void g1() { f(MACRO); }
void g2() { f(12); }
