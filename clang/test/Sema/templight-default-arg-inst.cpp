// RUN: %clang_cc1 -templight-dump %s 2>&1 | FileCheck %s
template<class T, class U = T>
class A {};

// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A::U'$}}
// CHECK: {{^kind:[ ]+DefaultTemplateArgumentInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+61]]{{:1'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A::U'$}}
// CHECK: {{^kind:[ ]+DefaultTemplateArgumentInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+56]]{{:1'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A::U'$}}
// CHECK: {{^kind:[ ]+DefaultTemplateArgumentChecking$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+50]]{{:6'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A::U'$}}
// CHECK: {{^kind:[ ]+DefaultTemplateArgumentChecking$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+45]]{{:6'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int, int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+39]]{{:8'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int, int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+34]]{{:8'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int, int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+28]]{{:8'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int, int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+23]]{{:8'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int, int>'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+17]]{{:8'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int, int>'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+12]]{{:8'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int, int>'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+6]]{{:8'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int, int>'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-arg-inst.cpp:}}[[@LINE+1]]{{:8'$}}
A<int> a;
