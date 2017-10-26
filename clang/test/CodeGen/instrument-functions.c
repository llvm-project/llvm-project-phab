// RUN: %clang_cc1 -S -debug-info-kind=standalone -emit-llvm -o - %s -finstrument-functions | FileCheck --check-prefix=DEFAULT --check-prefix=CHECK %s
// RUN: %clang_cc1 -S -debug-info-kind=standalone -emit-llvm -o - %s -finstrument-functions -fno-cygprofile-exit | FileCheck --check-prefix=NOEXIT --check-prefix=CHECK %s
// RUN: %clang_cc1 -S -debug-info-kind=standalone -emit-llvm -o - %s -finstrument-functions -fno-cygprofile-args | FileCheck --check-prefix=NOARGS --check-prefix=CHECK %s

// CHECK-LABEL: @test1
int test1(int x) {
// DEFAULT: call void @__cyg_profile_func_enter(i8*{{.*}}, i8*{{.*}}){{.*}}, !dbg
// DEFAULT: call void @__cyg_profile_func_exit(i8*{{.*}}, i8*{{.*}}){{.*}}, !dbg

// NOEXIT: call void @__cyg_profile_func_enter(i8*{{.*}}, i8*{{.*}}){{.*}}, !dbg
// NOEXIT-NOT: @__cyg_profile_func_exit

// NOARGS: call void @__cyg_profile_func_enter(){{.*}}, !dbg
// NOARGS: call void @__cyg_profile_func_exit(){{.*}}, !dbg

// CHECK: ret i32 %0
  return x;
}

// CHECK-LABEL: @test2
int test2(int) __attribute__((no_instrument_function));
int test2(int x) {
// CHECK-NOT: __cyg_profile_func_enter
// CHECK-NOT: __cyg_profile_func_exit
// CHECK: ret i32 %0
  return x;
}
