// REQUIRES: shell

//--- Invocation qqq-clang-g++ tries to find config file qqq-clang-g++.cfg first ...
//
// RUN: mkdir -p %T/testdmode
// RUN: [ ! -s %T/testdmode/qqq-clang-g++ ] || rm %T/testdmode/qqq-clang-g++
// RUN: ln -s %clang %T/testdmode/qqq-clang-g++
// RUN: echo "-Wundefined-func-template" > %T/testdmode/qqq-clang-g++.cfg
// RUN: echo "-Werror" > %T/testdmode/qqq.cfg
//
// RUN: %T/testdmode/qqq-clang-g++ -c -no-canonical-prefixes %s -### 2>&1 | FileCheck %s -check-prefix FULL-NAME
//
// FULL-NAME: Configuration file: {{.*}}/testdmode/qqq-clang-g++.cfg
// FULL-NAME: -Wundefined-func-template
// FULL-NAME-NOT: -Werror
//
//--- ... and qqq.cfg if qqq-clang-g++.cfg is not found.
//
// RUN: rm %T/testdmode/qqq-clang-g++.cfg
//
// RUN: %T/testdmode/qqq-clang-g++ -c -no-canonical-prefixes %s -### 2>&1 | FileCheck %s -check-prefix SHORT-NAME
//
// SHORT-NAME: Configuration file: {{.*}}/testdmode/qqq.cfg
// SHORT-NAME: -Werror
// SHORT-NAME-NOT: -Wundefined-func-template


//--- Config files are searched for in binary directory as well.
//
// RUN: [ ! -s %T/testbin/clang ] || rm %T/testbin/clang
// RUN: ln -s %clang %T/testbin/clang
// RUN: echo "-Werror" > %T/testbin/aaa.cfg
//
// RUN: %T/testbin/clang --config aaa.cfg -c -no-canonical-prefixes %s -### 2>&1 | FileCheck %s -check-prefix CHECK-BIN
//
// CHECK-BIN: Configuration file: {{.*}}/testbin/aaa.cfg
// CHECK-BIN: -Werror


//--- If command line contains options that change triple (for instance, -m32), clang tries
//    reloading config file.

//--- When reloading config file, target-clang-g++ tries to find config target32-clang-g++.cfg first ...
//
// RUN: mkdir -p %T/testreload
// RUN: [ ! -s %T/testreload/x86_64-clang-g++ ] || rm %T/testreload/x86_64-clang-g++
// RUN: ln -s %clang %T/testreload/x86_64-clang-g++
// RUN: echo "-Wundefined-func-template" > %T/testreload/i386-clang-g++.cfg
// RUN: echo "-Werror" > %T/testreload/i386.cfg
//
// RUN: %T/testreload/x86_64-clang-g++ -c -m32 -no-canonical-prefixes %s -### 2>&1 | FileCheck %s -check-prefix CHECK-RELOAD
//
// CHECK-RELOAD: Configuration file: {{.*}}/testreload/i386-clang-g++.cfg
// CHECK-RELOAD: -Wundefined-func-template
// CHECK-RELOAD-NOT: -Werror

//--- and target32.cfg if target32-g++.cfg is not found.
//
// RUN: rm %T/testreload/i386-clang-g++.cfg
//
// RUN: %T/testreload/x86_64-clang-g++ -c -m32 -no-canonical-prefixes %s -### 2>&1 | FileCheck %s -check-prefix CHECK-RELOAD2
//
// CHECK-RELOAD2: Configuration file: {{.*}}/testreload/i386.cfg
// CHECK-RELOAD2: -Werror
// CHECK-RELOAD2-NOT: -Wundefined-func-template
