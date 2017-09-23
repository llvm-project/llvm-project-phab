// RUN: %clang_cc1 -w -triple i386-linux-gnu -fsanitize=function -emit-llvm -o - %s | FileCheck %s --check-prefixes=X32
// RUN: %clang_cc1 -w -triple x86_64-linux-gnu -fsanitize=function -emit-llvm -o - %s | FileCheck %s --check-prefixes=X64

struct S {};

// X32: [[no_proto_ti:@.*]] = private constant i8* inttoptr (i32 4 to i8*)
// X64: [[no_proto_ti:@.*]] = private constant i8* inttoptr (i64 4 to i8*)

// X32: prologue <{ i32, i32 }> <{ i32 846595819, i32 sub (i32 ptrtoint (i8** [[no_proto_ti]] to i32), i32 ptrtoint (void ()* @no_proto to i32)) }>
// X64: prologue <{ i32, i32 }> <{ i32 846595819, i32 trunc (i64 sub (i64 ptrtoint (i8** [[no_proto_ti]] to i64), i64 ptrtoint (void ()* @no_proto to i64)) to i32) }>
void no_proto() {}

void proto(void) {}

typedef struct S (*vfunc0)(void);
typedef void (*vfunc1)(void);
typedef char (*vfunc2)(void);
typedef short (*vfunc3)(void);
typedef int (*vfunc4)(void);
typedef long long (*vfunc5)(void);
typedef float (*vfunc6)(void);
typedef double (*vfunc7)(void);
typedef void (*vfunc8)(int, int, int, int, int, int, int, int, int, int, int);

// X64-LABEL: @call_proto
void call_proto(void) {
  // X64: [[ICMP:%.*]] = icmp eq i8* {{.*}}, null, !nosanitize
  // X64-NEXT: br i1 [[ICMP]], {{.*}} !nosanitize
  vfunc0 f0 = &proto;
  f0();

  // X64: [[ICMP:%.*]] = icmp eq i8* {{.*}}, inttoptr (i64 4 to i8*), !nosanitize
  // X64-NEXT: br i1 [[ICMP]], {{.*}} !nosanitize
  vfunc1 f1 = &proto;
  f1();

  // X64: [[ICMP:%.*]] = icmp eq i8* {{.*}}, inttoptr (i64 16 to i8*), !nosanitize
  // X64-NEXT: br i1 [[ICMP]], {{.*}} !nosanitize
  vfunc2 f2 = &proto;
  f2();

  // X64: [[ICMP:%.*]] = icmp eq i8* {{.*}}, inttoptr (i64 20 to i8*), !nosanitize
  // X64-NEXT: br i1 [[ICMP]], {{.*}} !nosanitize
  vfunc3 f3 = &proto;
  f3();

  // X64: [[ICMP:%.*]] = icmp eq i8* {{.*}}, inttoptr (i64 24 to i8*), !nosanitize
  // X64-NEXT: br i1 [[ICMP]], {{.*}} !nosanitize
  vfunc4 f4 = &proto;
  f4();

  // X64: [[ICMP:%.*]] = icmp eq i8* {{.*}}, inttoptr (i64 28 to i8*), !nosanitize
  // X64-NEXT: br i1 [[ICMP]], {{.*}} !nosanitize
  vfunc5 f5 = &proto;
  f5();

  // X64: [[ICMP:%.*]] = icmp eq i8* {{.*}}, inttoptr (i64 8 to i8*), !nosanitize
  // X64-NEXT: br i1 [[ICMP]], {{.*}} !nosanitize
  vfunc6 f6 = &proto;
  f6();

  // X64: [[ICMP:%.*]] = icmp eq i8* {{.*}}, inttoptr (i64 12 to i8*), !nosanitize
  // X64-NEXT: br i1 [[ICMP]], {{.*}} !nosanitize
  vfunc7 f7 = &proto;
  f7();

  // X64: [[ICMP:%.*]] = icmp eq i8* {{.*}}, inttoptr (i64 3681400516 to i8*), !nosanitize
  // X64-NEXT: br i1 [[ICMP]], {{.*}} !nosanitize
  vfunc8 f8 = &proto;
  f8(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
}

// X64-LABEL: @call_no_proto
void call_no_proto(void) {
  // X64: [[ICMP:%.*]] = icmp eq i8* {{.*}}, null, !nosanitize
  // X64-NEXT: br i1 [[ICMP]], {{.*}} !nosanitize
  vfunc0 f0 = &no_proto;
  f0();
}

// X64-LABEL: @main
int main() {
  call_proto();
  call_no_proto();
  return 0;
}
