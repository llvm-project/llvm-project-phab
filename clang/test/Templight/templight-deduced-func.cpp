// RUN: %clang_cc1 -templight-dump %s 2>&1 | FileCheck %s

template <class T>
int foo(T){return 0;}

// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+foo$}}
// CHECK: {{^kind:[ ]+DeducedTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-deduced-func.cpp:}}[[@LINE+28]]{{:12'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+foo$}}
// CHECK: {{^kind:[ ]+DeducedTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-deduced-func.cpp:}}[[@LINE+23]]{{:12'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'foo<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-deduced-func.cpp:}}[[@LINE+17]]{{:12'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'foo<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-deduced-func.cpp:}}[[@LINE+12]]{{:12'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'foo<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-deduced-func.cpp:}}[[@LINE+6]]{{:12'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'foo<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-deduced-func.cpp:}}[[@LINE+1]]{{:12'$}}
int gvar = foo(0);
