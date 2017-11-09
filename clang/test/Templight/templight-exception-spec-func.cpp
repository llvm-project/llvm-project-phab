// RUN: %clang_cc1 -templight-dump -std=c++14 %s 2>&1 | FileCheck %s
template <bool B>
void f() noexcept(B) {}

int main()
{

// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+f$}}
// CHECK: {{^kind:[ ]+ExplicitTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-exception-spec-func.cpp:}}[[@LINE+50]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+f$}}
// CHECK: {{^kind:[ ]+ExplicitTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-exception-spec-func.cpp:}}[[@LINE+45]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+f$}}
// CHECK: {{^kind:[ ]+DeducedTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-exception-spec-func.cpp:}}[[@LINE+39]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+f$}}
// CHECK: {{^kind:[ ]+DeducedTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-exception-spec-func.cpp:}}[[@LINE+34]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'f<false>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-exception-spec-func.cpp:}}[[@LINE+28]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'f<false>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-exception-spec-func.cpp:}}[[@LINE+23]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'f<false>'$}}
// CHECK: {{^kind:[ ]+ExceptionSpecInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-exception-spec-func.cpp:}}[[@LINE+17]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'f<false>'$}}
// CHECK: {{^kind:[ ]+ExceptionSpecInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-exception-spec-func.cpp:}}[[@LINE+12]]{{:3'$}}
//
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'f<false>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^poi:[ ]+'.*templight-exception-spec-func.cpp:}}[[@LINE+6]]{{:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'f<false>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^poi:[ ]+'.*templight-exception-spec-func.cpp:}}[[@LINE+1]]{{:3'$}}
  f<false>();
}
