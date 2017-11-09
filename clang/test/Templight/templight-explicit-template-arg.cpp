// RUN: %clang_cc1 -templight-dump %s 2>&1 | FileCheck %s
template <class T>
void f(){}

int main()
{
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+f$}}
// CHECK: {{^kind:[ ]+ExplicitTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-explicit-template-arg.cpp:}}[[@LINE+39]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+f$}}
// CHECK: {{^kind:[ ]+ExplicitTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-explicit-template-arg.cpp:}}[[@LINE+34]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+f$}}
// CHECK: {{^kind:[ ]+DeducedTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-explicit-template-arg.cpp:}}[[@LINE+28]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+f$}}
// CHECK: {{^kind:[ ]+DeducedTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-explicit-template-arg.cpp:}}[[@LINE+23]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'f<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-explicit-template-arg.cpp:}}[[@LINE+17]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'f<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-explicit-template-arg.cpp:}}[[@LINE+12]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'f<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-explicit-template-arg.cpp:}}[[@LINE+6]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'f<int>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-explicit-template-arg.cpp:}}[[@LINE+1]]{{:3'$}}
  f<int>();
}
