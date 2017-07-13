// RUN: %clang -fno-destroy-globals -### %s 2>&1 | FileCheck %s --check-prefix=CHECK-FLAG-DISABLE
// RUN: %clang -fdestroy-globals -### %s 2>&1    | FileCheck %s --check-prefix=CHECK-FLAG-ENABLE1
// RUN: %clang -### %s 2>&1                      | FileCheck %s --check-prefix=CHECK-FLAG-ENABLE2

// RUN: %clang_cc1 -fno-destroy-globals -emit-llvm -o - %s | FileCheck %s --check-prefix=CHECK-CODE-DISABLE
// RUN: %clang_cc1 -fdestroy-globals -emit-llvm -o - %s    | FileCheck %s --check-prefix=CHECK-CODE-ENABLE

// CHECK-FLAG-DISABLE: "-cc1"
// CHECK-FLAG-DISABLE: "-fno-destroy-globals"

// CHECK-FLAG-ENABLE1: "-cc1"
// CHECK-FLAG-ENABLE1-NOT: "-fno-destroy-globals"

// CHECK-FLAG-ENABLE2: "-cc1"
// CHECK-FLAG-ENABLE2-NOT: "-fno-destroy-globals"

// CHECK-CODE-DISABLE-LABEL: define {{.*}} @__cxx_global_var_init
// CHECK-CODE-DISABLE: call void @_ZN1AC1Ev{{.*}}
// CHECK-CODE-DISABLE: ret void

// CHECK-CODE-ENABLE-LABEL: define {{.*}} @__cxx_global_var_init
// CHECK-CODE-ENABLE: call void @_ZN1AC1Ev{{.*}}
// CHECK-CODE-ENABLE: %{{.*}} = call i32 @__cxa_atexit{{.*}}_ZN1AD1Ev
// CHECK-CODE-ENABLE: ret void

struct A {
  virtual ~A() {}
};

A a;
