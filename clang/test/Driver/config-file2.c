// Due to ln -sf:
// REQUIRES: shell

// RUN: mkdir -p %t.cfgtest
// RUN: PWD=`pwd`
// RUN: cd %t.cfgtest
// RUN: ln -sf %clang test123-clang
// RUN: echo "-ferror-limit=666" > test123.cfg
// RUN: cd $PWD
// RUN: %t.cfgtest/test123-clang -v -c %s -o - 2>&1 | FileCheck %s

// CHECK: Configuration file:{{.*}}test123.cfg
// CHECK: -ferror-limit{{.*}}666
