// RUN: %clang -target arm-none-eabi -frwpi -c %s -o %t
// RUN: %clang -target arm-none-eabi -fropi -c %s -o %t
// RUN: %clang -target arm-none-eabi -fropi -frwpi -c %s -o %t
// RUN: %clang -target armeb-none-eabi -fropi -c %s -o %t
// RUN: %clang -target thumb-none-eabi -fropi -c %s -o %t
// RUN: %clang -target thumbeb-none-eabi -fropi -c %s -o %t
// RUN-NOT: %clang -target x86_64-linux-gnu -fropi  -c %s -o %t
