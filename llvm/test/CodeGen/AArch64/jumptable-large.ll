; RUN: llc -O0 -relocation-model=pic -code-model=small -march=aarch64 %s -o - | FileCheck --check-prefix=CHECK --check-prefix=CHECK-SMALL %s
; RUN: llc -O0 -relocation-model=pic -code-model=large -march=aarch64 %s -o - | FileCheck --check-prefix=CHECK --check-prefix=CHECK-LARGE %s

define double @f(i64) {
top:
  switch i64 %0, label %L19 [
    i64 0, label %if
    i64 1, label %if1
    i64 2, label %if2
    i64 3, label %if3
  ]

if:                                               ; preds = %top
  %1 = call double @g1(double -1.000000e+00)
  ret double %1

if1:                                              ; preds = %top
  %2 = call double @g2(double 1.000000e+00)
  ret double %2

if2:                                              ; preds = %top
  %3 = call double @g3(double 1.000000e+00)
  ret double %3

if3:                                              ; preds = %top
  %4 = call double @g4(double 1.000000e+00)
  ret double %4

L19:                                              ; preds = %top
  %5 = call double @g5(double 1.000000e+00)
  ret double %5
; CHECK-LABEL: .LJTI0_0:
; CHECK-LARGE-NEXT: .xword .LBB{{.*}}-.LJTI0_0
; CHECK-LARGE-NEXT: .xword .LBB{{.*}}-.LJTI0_0
; CHECK-LARGE-NEXT: .xword .LBB{{.*}}-.LJTI0_0
; CHECK-LARGE-NEXT: .xword .LBB{{.*}}-.LJTI0_0
; CHECK-SMALL-NEXT: .word .LBB{{.*}}-.LJTI0_0
; CHECK-SMALL-NEXT: .word .LBB{{.*}}-.LJTI0_0
; CHECK-SMALL-NEXT: .word .LBB{{.*}}-.LJTI0_0
; CHECK-SMALL-NEXT: .word .LBB{{.*}}-.LJTI0_0
}

declare double @g1(double)

declare double @g2(double)

declare double @g3(double)

declare double @g4(double)

declare double @g5(double)
