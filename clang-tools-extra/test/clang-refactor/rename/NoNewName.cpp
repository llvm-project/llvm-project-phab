// Check for an error while -new-name argument has not been passed to
// clang-refactor rename.
// RUN: not clang-refactor rename -offset=133 %s 2>&1 | FileCheck %s
// CHECK: clang-refactor rename: -new-name or -input is required.
