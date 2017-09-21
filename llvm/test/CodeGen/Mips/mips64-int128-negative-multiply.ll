; RUN: llc -mtriple=mips64-unknown-linux-gnu < %s
; REQUIRES: asserts

define void @mul_negative(i128* nocapture %in) {
entry:
  %0 = load i128, i128* %in, align 16
  %sub = mul i128 %0, -65535
  store i128 %sub, i128* %in, align 16
  ret void
}
