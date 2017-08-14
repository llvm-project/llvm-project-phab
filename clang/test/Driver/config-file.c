//--- Config file (full path) in output of -###
//
// RUN: %clang --config %S/Inputs/config-1.cfg -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-HHH
// CHECK-HHH: Target: x86_64-apple-darwin
// CHECK-HHH: Configuration file: {{.*}}Inputs{{.}}config-1.cfg
// CHECK-HHH: -Werror
// CHECK-HHH: -std=c99


//--- Nested config files
//
// RUN: %clang --config %S/Inputs/config-2.cfg -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-HHH2
// CHECK-HHH2: Target: x86_64-unknown-linux
// CHECK-HHH2: Configuration file: {{.*}}Inputs{{.}}config-2.cfg
// CHECK-HHH2: -Wundefined-func-template
//

// RUN: %clang --config %S/Inputs/config-2a.cfg -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-HHH2a
// CHECK-HHH2a: Target: x86_64-unknown-linux
// CHECK-HHH2a: Configuration file: {{.*}}Inputs{{.}}config-2a.cfg
// CHECK-HHH2a: -isysroot
// CHECK-HHH2a-SAME: /opt/data


//--- If config file isspecified by relative path (workdir/cfg-s2), it is searched for by that path.
//
// RUN: mkdir -p %T/workdir
// RUN: echo "@subdir/cfg-s2" > %T/workdir/cfg-1
// RUN: mkdir -p %T/workdir/subdir
// RUN: echo "-Wundefined-var-template" > %T/workdir/subdir/cfg-s2
//
// RUN: ( cd %T && %clang --config workdir/cfg-1 -c %s -### 2>&1 | FileCheck %s -check-prefix CHECK-REL )
//
// CHECK-REL: Configuration file: {{.*}}/workdir/cfg-1
// CHECK-REL: -Wundefined-var-template


//--- Config file (full path) in output of -v
//
// RUN: %clang --config %S/Inputs/config-1.cfg -c %s -v 2>&1 | FileCheck %s -check-prefix CHECK-V
// CHECK-V: Target: x86_64-apple-darwin
// CHECK-V: Configuration file: {{.*}}Inputs{{.}}config-1.cfg
// CHECK-V: -triple{{.*}}x86_64-apple-


//--- Unused options in config file do not produce warnings
//
// RUN: %clang --config %S/Inputs/config-4.cfg -c %s -v 2>&1 | FileCheck %s -check-prefix CHECK-UNUSED
// CHECK-UNUSED-NOT: argument unused during compilation:

