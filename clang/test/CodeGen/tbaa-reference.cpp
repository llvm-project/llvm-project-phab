// RUN: %clang_cc1 -triple x86_64-linux -O1 -disable-llvm-passes %s -emit-llvm -o - | FileCheck %s
//
// Check that we generate correct TBAA information for reference accesses.

struct S;

struct B {
  S &s;
  B(S &s) : s(s) {}
  void bar();
};

void foo(S &s) {
  B b(s);
  b.bar();
}

// CHECK-LABEL: _Z3fooR1S
// CHECK: store %struct.S* {{.*}}, %struct.S** {{.*}}, !tbaa [[TAG_pointer:!.*]]
//
// CHECK-DAG: [[TAG_pointer]] = !{[[TYPE_pointer:!.*]], [[TYPE_pointer]], i64 0}
// CHECK-DAG: [[TYPE_pointer]] = !{!"any pointer", [[TYPE_char:!.*]], i64 0}
// CHECK-DAG: [[TYPE_char]] = !{!"omnipotent char", {{!.*}}, i64 0}
