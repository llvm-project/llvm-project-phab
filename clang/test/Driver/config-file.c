//--- Config file (full path) in output of -###
//
// RUN: %clang --config %S/Inputs/config-1.cfg -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-HHH
// CHECK-HHH: Target: x86_64-apple-darwin
// CHECK-HHH: Configuration file: {{.*}}/Inputs/config-1.cfg
// CHECK-HHH: -Werror
// CHECK-HHH: -std=c99

//--- Nested config files
//
// RUN: %clang --config %S/Inputs/config-2.cfg -### 2>&1 | FileCheck %s -check-prefix CHECK-HHH2
// CHECK-HHH2: Target: x86_64-unknown-linux
// CHECK-HHH2: Configuration file: {{.*}}/Inputs/config-2.cfg
// CHECK-HHH2: -Wundefined-func-template
// CHECK-HHH2: -isysroot /opt/data

//--- Unexistent config file (full path) in output of -###
//
// RUN: not %clang --config %S/Inputs/inexistent.cfg -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-INEX
// CHECK-INEX: Configuration file {{.*}}/Inputs/inexistent.cfg' specified by option '--config' cannot be found

//--- Unexistent config file (file name) in output of -###
//
// RUN: not %clang --config inexistent-config-file-for-tests.cfg -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-INEX-NOSEARCH
// CHECK-INEX-NOSEARCH: Configuration {{.*}}inexistent-config-file-for-tests.cfg' specified by option '--config' cannot be found in directories:
// CHECK-INEX-NOSEARCH: ~/.llvm
// RUN: not %clang --config inexistent-config-file-for-tests.cfg -c %s -### 2>&1 | grep '%bindir'

//--- Config file (full path) in output of -v
//
// RUN: %clang --config %S/Inputs/config-1.cfg -c %s -v 2>&1 | FileCheck %s -check-prefix CHECK-V
// CHECK-V: Target: x86_64-apple-darwin
// CHECK-V: Configuration file: {{.*}}/Inputs/config-1.cfg
// CHECK-V: -triple{{.*}}x86_64-apple-
