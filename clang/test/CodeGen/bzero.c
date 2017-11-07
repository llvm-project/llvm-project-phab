// RUN: %clang_cc1 -emit-llvm < %s| FileCheck %s

typedef __SIZE_TYPE__ size_t;
void bzero(void*, size_t);
void foo(void);
  // CHECK: @test_bzero
  // CHECK: call void @llvm.memset
  // CHECK: call void @llvm.memset
  // CHECK-NOT: phi
void test_bzero() {
  char dst[20];
  int _sz = 20, len = 20;
  return (_sz  
          ? ((_sz >= len)
             ? bzero(dst, len)
             : foo())
          : bzero(dst, len));
}
