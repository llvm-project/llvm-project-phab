// REQUIRES: x86-registered-target
// RUN: %clang_cc1 %s -triple x86_64-apple-darwin10 -fasm-blocks -emit-llvm -o - | FileCheck %s

int gVar;
void t1() {
  __asm mov rax, 0xFF + offset gVar - 0xFE * 1
  // CHECK: t1
  // CHECK: mov rax, offset gVar + $$1
}

void t2() {
  __asm mov edx, offset t2
  // CHECK: t2
  // CHECK: mov edx, offset t2
}

void t3() {
  __asm _t3: mov eax, offset _t3
  // CHECK: t3
  // CHECK: {{.*}}__MSASMLABEL_.${:uid}___t3:
  // CHECK: mov eax, offset {{.*}}__MSASMLABEL_.${:uid}___t3
}

void t4() {
  __asm mov rbx, qword ptr 0x128[rax + offset t4 + rcx * 2]
  // CHECK: t4
  // CHECK: mov rbx, qword ptr [rax + rcx * $$2 + offset t4 + $$296]
}

