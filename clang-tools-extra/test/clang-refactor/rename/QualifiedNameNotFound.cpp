// RUN: not clang-refactor rename  -new-name=Foo -qualified-name=Bar %s -- 2>&1 | FileCheck %s
// CHECK: USREngine: could not find symbol Bar.
