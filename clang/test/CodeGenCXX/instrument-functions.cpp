// RUN: %clang_cc1 -S -emit-llvm -triple %itanium_abi_triple -o - %s -finstrument-functions | FileCheck %s

// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions | FileCheck %s
// RUN: %clang -S -emit-llvm -o - %s -fno-instrument-functions | FileCheck %s --check-prefix=NOINSTR

// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-file-list=instrument | FileCheck %s --check-prefix=NOFILE
// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-function-list=test3 | FileCheck %s --check-prefix=NOFUNC

// Below would see if mangled name partially matches. exclude-function-list matches demangled names, thus we expect to see instrument calls in test3.
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
// NOFUNC2: __cyg_profile_func_enter
// NOFUNC2: __cyg_profile_func_exit
// NOFUNC2: ret
  return x;
}

// Check function overload
// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-function-list=test3 | FileCheck %s --check-prefix=OVERLOAD
// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-function-list=Z5test3v | FileCheck %s --check-prefix=OVERLOAD1

// OVERLOAD: @_Z5test3v
// OVERLOAD1: @_Z5test3v
int test3() {
// OVERLOAD-NOT: __cyg_profile_func_enter
// OVERLOAD-NOT: __cyg_profile_func_exit
// OVERLOAD: ret
// OVERLOAD1: __cyg_profile_func_enter
// OVERLOAD1: __cyg_profile_func_exit
// OVERLOAD1: ret
  return 1;
}

template <class T>
T test4(T a) {
  return a;
}

// Check function with template specialization
// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-function-list=test4 | FileCheck %s --check-prefix=TPL
// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-function-list=Z5test4Ii | FileCheck %s --check-prefix=TPL1
// RUN: %clang -S -emit-llvm -o - %s -finstrument-functions -finstrument-functions-exclude-function-list=Z5test4Is | FileCheck %s --check-prefix=TPL2

// TPL: @_Z5test4IiET_S0_
// TPL1: @_Z5test4IiET_S0_
template<>
int test4<int>(int a) {
// TPL-NOT: __cyg_profile_func_enter
// TPL-NOT: __cyg_profile_func_exit
// TPL: ret
// TPL1: __cyg_profile_func_enter
// TPL1: __cyg_profile_func_exit
// TPL1: ret
  return a;
}

// TPL: @_Z5test4IsET_S0_
// TPL2: @_Z5test4IsET_S0_
template<>
short test4<short>(short a) {
// TPL-NOT: __cyg_profile_func_enter
// TPL-NOT: __cyg_profile_func_exit
// TPL: ret
// TPL2: __cyg_profile_func_enter
// TPL2: __cyg_profile_func_exit
// TPL2: ret
  return a;
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

