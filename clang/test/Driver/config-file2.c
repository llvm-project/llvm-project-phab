// REQUIRES: shell

//--- Invocation qqq-clang tries to find config file qqq.cfg
//
// RUN: mkdir -p %T/testbin
// RUN: [ ! -s %T/testbin/qqq-clang ] || rm %T/testbin/qqq-clang
// RUN: ln -s %clang %T/testbin/qqq-clang
// RUN: echo "-Wundefined-func-template" > %T/testbin/qqq.cfg
// RUN: %T/testbin/qqq-clang -c %s -### 2>&1 | FileCheck %s
// CHECK: Configuration file: {{.*}}/testbin/qqq.cfg
// CHECK: -Wundefined-func-template

//--- Config files are searched for in binary directory as well.
//
// RUN: [ ! -s %T/testbin/clang ] || rm %T/testbin/clang
// RUN: ln -s %clang %T/testbin/clang
// RUN: echo "-Werror" > %T/testbin/aaa.cfg
// RUN: %T/testbin/clang --config aaa.cfg -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-BIN
// CHECK-BIN: Configuration file: {{.*}}/testbin/aaa.cfg
// CHECK-BIN: -Werror

// RUN: mkdir -p %T/workdir
// RUN: echo "@subdir/cfg-s2" > %T/workdir/cfg-1
// RUN: mkdir -p %T/workdir/subdir
// RUN: echo "-Wundefined-var-template" > %T/workdir/subdir/cfg-s2
// RUN: ( cd %T && %clang --config workdir/cfg-1 -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-REL )
// CHECK-REL: Configuration file: {{.*}}/workdir/cfg-1
// CHECK-REL: -Wundefined-var-template

