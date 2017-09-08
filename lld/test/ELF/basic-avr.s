# REQUIRES: avr
# RUN: llvm-mc -filetype=obj -triple=avr-unknown-linux -mcpu=atmega328p %s -o %t.o
# RUN: ld.lld %t.o -o %t.exe -Ttext=0
# RUN: llvm-objdump -d %t.exe | FileCheck %s

main:
  call foo
  ldi r17, lo8(foo)
  ldi r18, hi8(bar)
  ldi r19, foo
foo:
  jmp foo
bar:
  call foo

# CHECK:      main:
# CHECK-NEXT:   0: 0e 94 05 00 <unknown>
# CHECK-NEXT:   4: 1a e0 <unknown>
# CHECK-NEXT:   6: 20 e0 <unknown>
# CHECK-NEXT:   8: 3a e0 <unknown>
# CHECK:      foo:
# CHECK-NEXT:   a: 0c 94 05 00 <unknown>
# CHECK:      bar:
# CHECK-NEXT:   e: 0e 94 05 00 <unknown>
