// Add function attribute "uwtable" for arm ehabi targets in C++.

// RUN: %clang -target arm-none-eabi  -### -S %s -o %t.s 2>&1 \
// RUN:    | FileCheck --check-prefix=CHECK-EABI %s
// CHECK-EABI: -munwind-tables

// RUN: %clang -target arm-linux-gnueabihf  -### -S %s -o %t.s 2>&1 \
// RUN:    | FileCheck --check-prefix=CHECK-GNU %s
// CHECK-GNU: -munwind-tables
