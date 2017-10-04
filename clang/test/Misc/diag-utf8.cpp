// RUN: not %clang_cc1 -fsyntax-only %s 2>&1 | FileCheck %s

struct Foo { int member; };

void f(Foo foo)
{
    "ideeen" << foo; // CHECK: {{.*[/\\]}}diag-utf8.cpp:7:14: error: invalid operands to binary expression ('const char *' and 'Foo')
    "ideëen" << foo; // CHECK: {{.*[/\\]}}diag-utf8.cpp:8:14: error: invalid operands to binary expression ('const char *' and 'Foo')
    "idez̈en" << foo; // CHECK: {{.*[/\\]}}diag-utf8.cpp:9:14: error: invalid operands to binary expression ('const char *' and 'Foo')
}
