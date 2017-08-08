// RUN: %clang_scudo %s -o %t
// RUN: rm -rf %t-dir
// RUN: mkdir %t-dir
// RUN: %run %t 100 > %t-dir/out1
// RUN: %run %t 100 > %t-dir/out2
// RUN: %run %t 10000 > %t-dir/out1
// RUN: %run %t 10000 > %t-dir/out2
// RUN: not diff %t-dir/out?
// RUN: rm -rf %t-dir
// UNSUPPORTED: i386-linux,i686-linux,arm-linux,armhf-linux,aarch64-linux,mips-linux,mipsel-linux,mips64-linux,mips64el-linux

// Tests that the allocator shuffles the chunks before returning to the user.

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
  int alloc_size = argc == 2 ? atoi(argv[1]) : 100;
  char *base = new char[alloc_size];
  for (int i = 0; i < 20; i++) {
    char *p = new char[alloc_size];
    printf("%zd\n", base - p);
  }
}
