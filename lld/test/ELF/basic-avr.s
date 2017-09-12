# REQUIRES: avr
# RUN: llvm-mc -filetype=obj -triple=avr-unknown-linux -mcpu=atmega328p %s -o %t.o
# RUN: ld.lld %t.o -o %t.exe -Ttext=0
# RUN: llvm-objdump -d %t.exe | FileCheck %s

main:
  call foo
  ldi r16, lo8(foo)
  ldi r17, hi8(foo)
  ldi r18, hh8(foo)
  ldi r19, hhi8(foo)
  ldi r20, foo
  ldi r21, lo8(-(foo))
  ldi r22, hi8(-(foo))
  ldi r23, hh8(-(foo))
  ldi r24, hhi8(-(foo))
  ldi r25, pm_lo8(foo)
  ldi r26, pm_hi8(foo)
  ldi r27, pm_hh8(foo)
  ldi r28, pm_lo8(-(foo))
  ldi r29, pm_hi8(-(foo))
  ldi r30, pm_hh8(-(foo))
  brne foo
  rjmp foo
foo:
  ldd r2, Y+2
  ldd r0, Y+0
  ldd r9, Z+12
  ldd r7, Z+30
  ldd r9, Z+foo
  lds r16, foo
  in r20, foo+1
  sbi main, 0
  adiw r30, foo

# CHECK:      main:
# CHECK-NEXT:   0: 0e 94 13 00 <unknown>
# CHECK-NEXT:   4: 06 e2 <unknown>
# CHECK-NEXT:   6: 10 e0 <unknown>
# CHECK-NEXT:   8: 20 e0 <unknown>
# CHECK-NEXT:   a: 30 e0 <unknown>
# CHECK-NEXT:   c: 46 e2 <unknown>
# CHECK-NEXT:   e: 5a ed <unknown>
# CHECK-NEXT:  10: 6f ef <unknown>
# CHECK-NEXT:  12: 7f ef <unknown>
# CHECK-NEXT:  14: 8f ef <unknown>
# CHECK-NEXT:  16: 93 e1 <unknown>
# CHECK-NEXT:  18: a0 e0 <unknown>
# CHECK-NEXT:  1a: b0 e0 <unknown>
# CHECK-NEXT:  1c: cd ee <unknown>
# CHECK-NEXT:  1e: df ef <unknown>
# CHECK-NEXT:  20: ef ef <unknown>
# CHECK-NEXT:  22: 09 f4 <unknown>
# CHECK-NEXT:  24: 00 c0 <unknown>
# CHECK:      foo:
# CHECK-NEXT:  26: 2a 80 <unknown>
# CHECK-NEXT:  28: 08 80 <unknown>
# CHECK-NEXT:  2a: 94 84 <unknown>
# CHECK-NEXT:  2c: 76 8c <unknown>
# CHECK-NEXT:  2e: 96 a0 <unknown>
# CHECK-NEXT:  30: 00 91 26 00 <unknown>
# CHECK-NEXT:  34: 47 b5 <unknown>
# CHECK-NEXT:  36: 00 9a <unknown>
# CHECK-NEXT:  38: b6 96 <unknown>
