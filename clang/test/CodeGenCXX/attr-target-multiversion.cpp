// RUN: %clang_cc1 -triple x86_64-linux-gnu -target-cpu x86-64 -emit-llvm %s -o - | FileCheck %s

struct S {
  __attribute__((target("arch=sandybridge")))
  int mv(){return 3;}
  __attribute__((target("arch=ivybridge")))
  int mv(){return 2;}
  __attribute__((target("default")))
  int mv(){ return 1;}
};

// CHECK: @_ZN1S2mvEv.ifunc = ifunc i{{[0-9]+}} (%struct.S*), i{{[0-9]+}} (%struct.S*)* ()* @_ZN1S2mvEv.resolver

// CHECK: define void @_Z3foov()
// CHECK: call i{{[0-9]+}} @_ZN1S2mvEv.ifunc(%struct.S* %
void foo() {
  S s;
  s.mv();
}

// CHECK: define i{{[0-9]+}} (%struct.S*)* @_ZN1S2mvEv.resolver
// CHECK: ret i{{[0-9]+}} (%struct.S*)* @_ZN1S2mvEv.arch_sandybridge
// CHECK: ret i{{[0-9]+}} (%struct.S*)* @_ZN1S2mvEv.arch_ivybridge
// CHECK: ret i{{[0-9]+}} (%struct.S*)* @_ZN1S2mvEv

// CHECK-DAG: define linkonce_odr i{{[0-9]+}} @_ZN1S2mvEv.arch_sandybridge(%struct.S* %this)
// CHECK-DAG: define linkonce_odr i{{[0-9]+}} @_ZN1S2mvEv.arch_ivybridge(%struct.S* %this)
// CHECK-DAG: define linkonce_odr i{{[0-9]+}} @_ZN1S2mvEv(%struct.S* %this)
