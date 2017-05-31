// RUN: not %clang -S -emit-llvm --target=aarch64 %s -o - 2>&1 | FileCheck %s --check-prefix=CHECK-ERROR

_Float16 a0 = 1.f166;
_Float16 a1 = 1.f1;

//CHECK-ERROR: error: invalid suffix 'f166' on floating constant
//CHECK-ERROR-NEXT: _Float16 a0 = 1.f166;
//CHECK-ERROR-NEXT:                 ^
//CHECK-ERROR-NEXT: error: invalid suffix 'f1' on floating constant
//CHECK-ERROR-NEXT: _Float16 a1 = 1.f1;
//CHECK-ERROR-NEXT:                 ^
