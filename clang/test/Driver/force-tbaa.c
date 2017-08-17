// RUN: %clang -fno-strict-aliasing -fforce-tbaa -### %s 2>&1 | FileCheck -check-prefix=CHECK-WITH-FORCE-TBAA %s
// RUN: %clang -fno-strict-aliasing -### %s 2>&1 | FileCheck -check-prefix=CHECK-WITHOUT-FORCE-TBAA %s

// CHECK-WITH-FORCE-TBAA: -cc1
// CHECK-WITH-FORCE-TBAA: -force-tbaa
// CHECK-WITH-FORCE-TBAA: -relaxed-aliasing

// CHECK-WITHOUT-FORCE-TBAA: -cc1
// CHECK-WITHOUT-FORCE-TBAA-NOT: -force-tbaa
// CHECK-WITHOUT-FORCE-TBAA: -relaxed-aliasing
