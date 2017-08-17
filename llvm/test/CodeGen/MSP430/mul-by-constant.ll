; RUN: llc < %s | FileCheck %s

target triple = "msp430-none-elf"

; CHECK-LABEL: foo16
; CHECK: rla.w
; CHECK: add.w
define zeroext i16 @foo16(i16) local_unnamed_addr {
  %2 = mul i16 %0, 3
  ret i16 %2
}

; CHECK-LABEL: bar16
; CHECK: rla.w
; CHECK: rla.w
; CHECK: rla.w
; CHECK: sub.w
define zeroext i16 @bar16(i16) local_unnamed_addr {
  %2 = mul i16 %0, 7
  ret i16 %2
}

; CHECK-LABEL: foo8
; CHECK: rla.b
; CHECK: add.b
define zeroext i8 @foo8(i8) local_unnamed_addr {
  %2 = mul i8 %0, 3
  ret i8 %2
}

; CHECK-LABEL: bar8
; CHECK: rla.b
; CHECK: rla.b
; CHECK: rla.b
; CHECK: sub.b
define zeroext i8 @bar8(i8) local_unnamed_addr {
  %2 = mul i8 %0, 7
  ret i8 %2
}

; CHECK-LABEL: large
; CHECK: call
define zeroext i16 @large(i16) local_unnamed_addr {
  %2 = mul i16 %0, 85
  ret i16 %2
}

; CHECK-LABEL: small
; CHECK: rla.w
define zeroext i16 @small(i16) local_unnamed_addr {
  %2 = mul i16 %0, 127
  ret i16 %2
}
