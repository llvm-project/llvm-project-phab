// REQUIRES: x86-registered-target
// RUN: %clang_cc1 %s -triple x86_64-apple-darwin10 -fasm-blocks -verify

void t1() {
  int lVar;
  __asm mov rax, offset lVar    // expected-error {{illegal operand for offset operator}}
}

void t2() {
  enum { A };
  __asm mov eax, offset A       // expected-error {{offset operator cannot yet handle constants}}
}
