; Test incoming i128 arguments.
;
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s

@a = local_unnamed_addr global i128 1, align 16
@b = common local_unnamed_addr global float 0.000000e+00, align 4

; Function Attrs: norecurse nounwind
define signext i32 @main() local_unnamed_addr #0 {
; CHECK-LABEL: main:
; CHECK:	lgrl	%r0, a+8
; CHECK:	stg	%r0, 168(%r15)
; CHECK:	lgrl	%r0, a
; CHECK:	stg	%r0, 160(%r15)
; CHECK:	la	%r2, 160(%r15)
; CHECK:        brasl	%r14, __floatuntisf@PLT
  %1 = load i128, i128* @a, align 16
  %2 = uitofp i128 %1 to float
  store float %2, float* @b, align 4
  ret i32 0
}
