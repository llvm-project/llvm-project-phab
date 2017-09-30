// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions | FileCheck %s
// RUN: %clang -S -emit-llvm -o - %s -fno-instrument-functions | FileCheck %s --check-prefix=NOINSTR

// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-file-list=instrument | FileCheck %s --check-prefix=NOFILE
// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-function-list=test3 | FileCheck %s --check-prefix=NOFUNC

// CHECK: @test1
// NOINSTR: @test1
// NOFILE: @test1
// NOFUNC: @test1
int test1(int x) {
// CHECK: __cyg_profile_func_enter
// CHECK: __cyg_profile_func_exit
// CHECK: ret
// NOINSTR-NOT: __cyg_profile_func_enter
// NOINSTR-NOT: __cyg_profile_func_exit
// NOINSTR: ret
// NOFILE-NOT: __cyg_profile_func_enter
// NOFILE-NOT: __cyg_profile_func_exit
// NOFILE: ret
// NOFUNC: __cyg_profile_func_enter
// NOFUNC: __cyg_profile_func_exit
// NOFUNC: ret
  return x;
}

// CHECK: @test2
// NOINSTR: @test2
// NOFILE: @test2
// NOFUNC: @test2
int test2(int) __attribute__((no_instrument_function));
int test2(int x) {
// CHECK-NOT: __cyg_profile_func_enter
// CHECK-NOT: __cyg_profile_func_exit
// CHECK: ret
// NOINSTR-NOT: __cyg_profile_func_enter
// NOINSTR-NOT: __cyg_profile_func_exit
// NOINSTR: ret
// NOFILE-NOT: __cyg_profile_func_enter
// NOFILE-NOT: __cyg_profile_func_exit
// NOFILE: ret
// NOFUNC-NOT: __cyg_profile_func_enter
// NOFUNC-NOT: __cyg_profile_func_exit
// NOFUNC: ret
  return x;
}

// CHECK: @test3
// NOINSTR: @test3
// NOFILE: @test3
// NOFUNC: @test3
int test3(int x) {
// CHECK: __cyg_profile_func_enter
// CHECK: __cyg_profile_func_exit
// CHECK: ret
// NOINSTR-NOT: __cyg_profile_func_enter
// NOINSTR-NOT: __cyg_profile_func_exit
// NOINSTR: ret
// NOFILE-NOT: __cyg_profile_func_enter
// NOFILE-NOT: __cyg_profile_func_exit
// NOFILE: ret
// NOFUNC-NOT: __cyg_profile_func_enter
// NOFUNC-NOT: __cyg_profile_func_exit
// NOFUNC: ret
  return x;
}
