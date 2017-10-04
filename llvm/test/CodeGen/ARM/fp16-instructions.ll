; RUN: llc < %s -mtriple=arm-none-eabi -float-abi=soft | FileCheck %s --check-prefix=CHECK-SOFT
; RUN: llc < %s -mtriple=arm-none-eabi -mattr=+vfp4 -float-abi=hard | FileCheck %s --check-prefix=CHECK-FP16
; RUN: llc < %s -mtriple=arm-none-eabi -mattr=+neon,+fullfp16 -float-abi=hard | FileCheck %s --check-prefix=CHECK-FULLFP16

define half @Sub(half %a, half %b) local_unnamed_addr {
entry:
;CHECK-SOFT-LABEL:      Sub:
;CHECK-SOFT:            bl  __aeabi_h2f
;CHECK-SOFT:            bl  __aeabi_h2f
;CHECK-SOFT:            bl  __aeabi_fsub
;CHECK-SOFT:            bl  __aeabi_f2h

;CHECK-FP16-LABEL:      Sub:
;CHECK-FP16:            vsub.f32  s0, s0, s2
;CHECK-FP16-NEXT:       mov pc, lr

;CHECK-FULLFP16-LABEL:  Sub:
;CHECK-FULLFP16:        vsub.f16  s0, s0, s1
;CHECK-FULLFP16-NEXT:   mov pc, lr

  %sub = fsub half %a, %b
  ret half %sub
}

define half @Add(half %a, half %b) local_unnamed_addr {
entry:
;CHECK-SOFT-LABEL:      Add:
;CHECK-SOFT:            bl  __aeabi_h2f
;CHECK-SOFT:            bl  __aeabi_h2f
;CHECK-SOFT:            bl  __aeabi_fadd
;CHECK-SOFT:            bl  __aeabi_f2h

;CHECK-FP16-LABEL:      Add:
;CHECK-FP16:            vadd.f32  s0, s0, s2
;CHECK-FP16-NEXT:       mov pc, lr

;CHECK-FULLFP16-LABEL:  Add:
;CHECK-FULLFP16:        vadd.f16  s0, s0, s1
;CHECK-FULLFP16-NEXT:   mov pc, lr

  %add = fadd half %a, %b
  ret half %add
}
