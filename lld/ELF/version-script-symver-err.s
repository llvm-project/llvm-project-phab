# REQUIRES: x86

# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t.o
# RUN: not ld.lld %t.o -o %t 2>&1 | FileCheck %s
# CHECK:      error: duplicate symbol: bar
# CHECK-NEXT:   >>> defined at {{.*}}.o
# CHECK-NEXT:   >>> defined at {{.*}}.o

.global _start
.global bar
.symver _start, bar@@VERSION
_start:
  jmp bar

bar:
