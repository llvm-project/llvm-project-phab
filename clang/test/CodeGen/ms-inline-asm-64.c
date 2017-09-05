// REQUIRES: x86-registered-target
// RUN: %clang_cc1 %s -triple x86_64-apple-darwin10 -fasm-blocks -emit-llvm -o - | FileCheck %s

struct t1_type { int a, b; };

int t1() {
  struct t1_type foo;
  foo.a = 1;
  foo.b = 2;
  __asm {
     lea ebx, foo
     mov eax, [ebx].0
     mov [ebx].4, ecx
  }
  return foo.b;
// CHECK: t1
// CHECK: call void asm sideeffect inteldialect
// CHECK-SAME: lea ebx, $0
// CHECK-SAME: mov eax, [ebx]
// CHECK-SAME: mov [ebx + $$4], ecx
// CHECK-SAME: "*m,~{eax},~{ebx},~{dirflag},~{fpsr},~{flags}"(%struct.t1_type* %{{.*}})
}

int t2() {
  struct t1_type foo;
  foo.a = 1;
  foo.b = 2;
  __asm {
     lea ebx, foo
     {
       mov eax, [ebx].foo.a
     }
     mov [ebx].foo.b, ecx
  }
  return foo.b;
// CHECK: t2
// CHECK: call void asm sideeffect inteldialect
// CHECK-SAME: lea ebx, $0
// CHECK-SAME: mov eax, [ebx]
// CHECK-SAME: mov [ebx + $$4], ecx
// CHECK-SAME: "*m,~{eax},~{ebx},~{dirflag},~{fpsr},~{flags}"(%struct.t1_type* %{{.*}})
}
