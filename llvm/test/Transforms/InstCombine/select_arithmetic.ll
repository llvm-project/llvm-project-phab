; RUN: opt < %s -instcombine -S | FileCheck %s

target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"

; Tests folding constants from two similar selects that feed a add
define float @test1(i1 zeroext %arg) #0 {
  %tmp = select i1 %arg, float 5.000000e+00, float 6.000000e+00
  %tmp1 = select i1 %arg, float 1.000000e+00, float 9.000000e+00
  %tmp2 = fadd float %tmp, %tmp1
  ret float %tmp2
; CHECK-LABEL: @test1(
; CHECK: %tmp2 = select i1 %arg, float 6.000000e+00, float 1.500000e+01
; CHECK-NOT: fadd
; CHECK: ret float %tmp2
}

; Tests folding constants from two similar selects that feed a sub
define float @test2(i1 zeroext %arg) #0 {
  %tmp = select i1 %arg, float 5.000000e+00, float 6.000000e+00
  %tmp1 = select i1 %arg, float 1.000000e+00, float 9.000000e+00
  %tmp2 = fsub float %tmp, %tmp1
  ret float %tmp2
; CHECK-LABEL: @test2(
; CHECK: %tmp2 = select i1 %arg, float 4.000000e+00, float -3.000000e+00
; CHECK-NOT: fsub
; CHECK: ret float %tmp2
}

; Tests folding constants from two similar selects that feed a mul
define float @test3(i1 zeroext %arg) #0 {
  %tmp = select i1 %arg, float 5.000000e+00, float 6.000000e+00
  %tmp1 = select i1 %arg, float 1.000000e+00, float 9.000000e+00
  %tmp2 = fmul float %tmp, %tmp1
  ret float %tmp2
; CHECK-LABEL: @test3(
; CHECK: %tmp2 = select i1 %arg, float 5.000000e+00, float 5.400000e+01
; CHECK-NOT: fmul
; CHECK: ret float %tmp2
}

; Tests not folding constants if the selects have more than one use.
declare void @use_double(double)
define double @test4(i1 zeroext %arg, double %div) {
  %tmp = select i1 %arg, double %div, double 5.000000e-03
  %mul = fmul contract double %tmp, %tmp
  call void @use_double(double %tmp)
  ret double %mul
; CHECK-LABEL: @test4(
; CHECK: [[TMP:%.*]] = select i1 %arg, double %div, double 5.000000e-03
; CHECK-NEXT: [[MUL:%.*]] = fmul contract double [[TMP]], [[TMP]]
; CHECK-NOT: fmul contract double %div, %div
; CHECK: ret double [[MUL]]
}
