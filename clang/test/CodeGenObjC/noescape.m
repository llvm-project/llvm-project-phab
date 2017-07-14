// RUN: %clang_cc1 -fblocks -emit-llvm -o - %s | FileCheck %s

typedef void (^BlockTy)(void);

union U {
  int *i;
  long long *ll;
} __attribute__((transparent_union));

void noescapeFunc0(id, __attribute__((noescape)) BlockTy);
void noescapeFunc1(__attribute__((noescape)) int *);
void noescapeFunc2(__attribute__((noescape)) id);
void noescapeFunc3(__attribute__((noescape)) union U);

// CHECK-LABEL: define void @test0(
// CHECK: call void @noescapeFunc0({{.*}}, [[TY0:.*]] nocapture {{.*}})
// CHECK: declare void @noescapeFunc0(i8*, [[TY0]] nocapture)
void test0(BlockTy b) {
  noescapeFunc0(0, b);
}

// CHECK-LABEL: define void @test1(
// CHECK: call void @noescapeFunc1([[TY1:.*]] nocapture {{.*}})
// CHECK: declare void @noescapeFunc1([[TY1]] nocapture)
void test1(int *i) {
  noescapeFunc1(i);
}

// CHECK-LABEL: define void @test2(
// CHECK: call void @noescapeFunc2([[TY2:.*]] nocapture {{.*}})
// CHECK: declare void @noescapeFunc2([[TY2]] nocapture)
void test2(id i) {
  noescapeFunc2(i);
}

// CHECK-LABEL: define void @test3(
// CHECK: call void @noescapeFunc3([[TY3:.*]] nocapture {{.*}})
// CHECK: declare void @noescapeFunc3([[TY3]] nocapture)
void test3(union U u) {
  noescapeFunc3(u);
}
