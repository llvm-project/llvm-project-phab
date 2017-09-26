// RUN: clang-tidy %s -- 2>&1 | FileCheck -implicit-check-not='{{warning:|error:}}' %s
// RUN: clang-tidy %s -checks='-*,cert-exp59-cpp' -- 2>&1 | FileCheck -implicit-check-not='{{warning:|error:}}' -check-prefix=CHECK2 %s
// RUN: clang-tidy %s -checks='-*,cert-exp59-cpp' -- -Wno-invalid-offsetof 2>&1 | FileCheck -implicit-check-not='{{warning:|error:}}' -check-prefix=CHECK2 %s

#include <cstddef>

struct D {
    virtual void f() {}
    int i;
};

void f() {
    size_t Off = offsetof(D, i);
    //CHECK: :[[@LINE-1]]:18: warning: offset of on non-POD type 'D' [clang-diagnostic-invalid-offsetof]
    //CHECK2: :[[@LINE-2]]:18: warning: offset of on non-POD type 'D' [cert-exp59-cpp]
}
