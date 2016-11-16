// RUN: %clang --config %S/Inputs/config-1.cfg -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-HHH
// CHECK-HHH: Target: x86_64-apple-darwin
// CHECK-HHH: Configuration file: {{.*}}/Inputs/config-1.cfg
// CHECK-HHH: -Werror
// CHECK-HHH: -std=c99

// RUN: not %clang --config %S/Inputs/inexistent.cfg -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-INEX
// CHECK-INEX: Configuration file {{.*}}/Inputs/inexistent.cfg' specified by option '--config' cannot be found

// RUN: not %clang --config inexistent.cfg -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-INEX-NOSEARCH
// CHECK-INEX-NOSEARCH: Configuration {{.*}}inexistent.cfg' specified by option '--config' cannot be found in directories:
// CHECK-INEX-NOSEARCH: ~/.llvm
// CHECK-INEX-NOSEARCH: /etc/llvm

// RUN: %clang --config %S/Inputs/config-2.cfg -### 2>&1 | FileCheck %s -check-prefix CHECK-HHH-NOFILE
// CHECK-HHH-NOFILE: Target: x86_64-unknown-linux
// CHECK-HHH-NOFILE: Configuration file: {{.*}}/Inputs/config-2.cfg

// RUN: %clang --config %S/Inputs/config-1.cfg -c %s -v 2>&1 | FileCheck %s -check-prefix CHECK-V
// CHECK-V: Target: x86_64-apple-darwin
// CHECK-V: Configuration file: {{.*}}/Inputs/config-1.cfg
// CHECK-V: -triple{{.*}}x86_64-apple-
