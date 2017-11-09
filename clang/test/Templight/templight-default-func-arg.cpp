// RUN: %clang_cc1 -std=c++14 -templight-dump %s 2>&1 | FileCheck %s
template <class T>
void foo(T b = 0) {};

int main()
{

// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+foo$}}
// CHECK: {{^kind:[ ]+ExplicitTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-func-arg.cpp:}}[[@LINE+50]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+foo$}}
// CHECK: {{^kind:[ ]+ExplicitTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-func-arg.cpp:}}[[@LINE+45]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+foo$}}
// CHECK: {{^kind:[ ]+DeducedTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-func-arg.cpp:}}[[@LINE+39]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+foo$}}
// CHECK: {{^kind:[ ]+DeducedTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-func-arg.cpp:}}[[@LINE+34]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'foo<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-func-arg.cpp:}}[[@LINE+28]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'foo<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-func-arg.cpp:}}[[@LINE+23]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+b$}}
// CHECK: {{^kind:[ ]+DefaultFunctionArgumentInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-func-arg.cpp:}}[[@LINE+17]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+b$}}
// CHECK: {{^kind:[ ]+DefaultFunctionArgumentInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-func-arg.cpp:}}[[@LINE+12]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'foo<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-default-func-arg.cpp:}}[[@LINE+6]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'foo<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-default-func-arg.cpp:}}[[@LINE+1]]{{:3'$}}
  foo<int>();
}
