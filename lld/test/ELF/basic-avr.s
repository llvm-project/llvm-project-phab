# REQUIRES: avr
# RUN: llvm-mc -filetype=obj -triple=avr-unknown-linux -mcpu=atmega328p %s -o %t.o
# RUN: ld.lld %t.o -o %t.exe -Ttext=0
# RUN: llvm-objdump -d %t.exe | FileCheck %s

main:
  ldi r17, lo8(foo)
  ldi r18, hi8(bar)
  ldi r19, hh8(foo)
  ldi r20, foo
  brne next
  rjmp next
  adiw r24, bar
next:
  jmp next
foo:
  jmp foo
bar:
  jmp bar

# CHECK:      main:
# CHECK-NEXT:   0: 12 e1 <unknown>
# CHECK-NEXT:   2: 20 e0 <unknown>
# CHECK-NEXT:   4: 30 e0 <unknown>
# CHECK-NEXT:   6: 42 e1 <unknown>
# CHECK-NEXT:   8: 11 f4 <unknown>
# CHECK-NEXT:   a: 01 c0 <unknown>
# CHECK-NEXT:   c: 46 96 <unknown>
# CHECK:      next:
# CHECK-NEXT:   e: 0c 94 07 00 <unknown>
# CHECK:      foo:
# CHECK-NEXT:  12: 0c 94 09 00 <unknown>
# CHECK:      bar:
# CHECK-NEXT:  16: 0c 94 0b 00 <unknown>
