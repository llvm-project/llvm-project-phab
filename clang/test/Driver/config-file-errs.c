//--- No more than one '--config' may be specified.
//
// RUN: not %clang --config 1.cfg --config 2.cfg 2>&1 | FileCheck %s -check-prefix CHECK-DUPLICATE
// CHECK-DUPLICATE: no more than one option '--config' is allowed


//--- '--config' must be followed by config file name.
//
// RUN: not %clang --config 2>&1 | FileCheck %s -check-prefix CHECK-MISSING-FILE
// CHECK-MISSING-FILE: argument to '--config' is missing (expected 1 value)


//--- Argument of '--config' must be existing file, if it is specified by path.
//
// RUN: not %clang --config somewhere/nonexistent-config-file 2>&1 | FileCheck %s -check-prefix CHECK-NONEXISTENT
// CHECK-NONEXISTENT: configuration file '{{.*}}somewhere/nonexistent-config-file' does not exist


//--- Argument of '--config' must exist somewhere is well-known directories, it is is specified by bare name.
//
// RUN: not %clang --config nonexistent-config-file 2>&1 | FileCheck %s -check-prefix CHECK-NOTFOUND
// CHECK-NOTFOUND: configuration file 'nonexistent-config-file.cfg' cannot be found
// CHECK-NOTFOUND: was searched for in the directory:


//--- Argument in config file cannot cross the file boundary
//
// RUN: not %clang --config %S/Inputs/config-5.cfg x86_64-unknown-linux-gnu -c %s 2>&1 | FileCheck %s -check-prefix CHECK-CROSS
// CHECK-CROSS: error: argument to '-target' is missing
