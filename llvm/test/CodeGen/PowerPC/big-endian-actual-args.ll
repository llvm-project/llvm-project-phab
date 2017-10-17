; RUN: llc -ppc-ignore-percent-prefix -verify-machineinstrs < %s \
; RUN:   -mtriple=powerpc-unknown-linux-gnu | \
; RUN:   grep "addc 4, 4, 6"
; RUN: llc -ppc-ignore-percent-prefix -verify-machineinstrs < %s \
; RUN:   -mtriple=powerpc-unknown-linux-gnu | \
; RUN:   grep "adde 3, 3, 5"

define i64 @foo(i64 %x, i64 %y) {
  %z = add i64 %x, %y
  ret i64 %z
}
