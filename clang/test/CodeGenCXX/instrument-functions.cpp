// RUN: %clang_cc1 -S -emit-llvm -triple %itanium_abi_triple -o - %s -finstrument-functions | FileCheck %s

// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions | FileCheck %s
// RUN: %clang -S -emit-llvm -o - %s -fno-instrument-functions | FileCheck %s --check-prefix=NOINSTR

// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-file-list=instrument | FileCheck %s --check-prefix=NOFILE
// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-function-list=test3 | FileCheck %s --check-prefix=NOFUNC
// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-function-list=Z5test3 | FileCheck %s --check-prefix=NOFUNC2

// CHECK: @_Z5test1i
// NOINSTR: @_Z5test1i
// NOFILE: @_Z5test1i
// NOFUNC: @_Z5test1i
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

// CHECK: @_Z5test2i
int test2(int) __attribute__((no_instrument_function));
int test2(int x) {
// CHECK-NOT: __cyg_profile_func_enter
// CHECK-NOT: __cyg_profile_func_exit
// CHECK: ret
  return x;
}

// CHECK: @_Z5test3i
// NOINSTR: @_Z5test3i
// NOFILE: @_Z5test3i
// NOFUNC: @_Z5test3i
// NOFUNC2: @_Z5test3i
int test3(int x){
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
// NOFUNC2: __cyg_profile_func_enter
// NOFUNC2: __cyg_profile_func_exit
// NOFUNC2: ret
  return x;
}

// This test case previously crashed code generation.  It exists solely
// to test -finstrument-function does not crash codegen for this trivial
// case.
namespace rdar9445102 {
  class Rdar9445102 {
    public:
      Rdar9445102();
  };
}
static rdar9445102::Rdar9445102 s_rdar9445102Initializer;

