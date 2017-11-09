// RUN: %clang_cc1 -templight-dump %s 2>&1 | FileCheck %s
template <class T = int>
class A {};

// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A::T'$}}
// CHECK: {{^kind:[ ]+DefaultTemplateArgumentChecking$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-template-arg.cpp:}}[[@LINE+50]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A::T'$}}
// CHECK: {{^kind:[ ]+DefaultTemplateArgumentChecking$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-template-arg.cpp:}}[[@LINE+45]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-template-arg.cpp:}}[[@LINE+39]]{{:5'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-template-arg.cpp:}}[[@LINE+34]]{{:5'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-template-arg.cpp:}}[[@LINE+28]]{{:5'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-template-arg.cpp:}}[[@LINE+23]]{{:5'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int>'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-template-arg.cpp:}}[[@LINE+17]]{{:5'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int>'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-template-arg.cpp:}}[[@LINE+12]]{{:5'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int>'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-template-arg.cpp:}}[[@LINE+6]]{{:5'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'A<int>'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-template-arg.cpp:}}[[@LINE+1]]{{:5'$}}
A<> a;
