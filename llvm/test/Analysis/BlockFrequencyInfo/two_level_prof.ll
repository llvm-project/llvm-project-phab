; RUN: opt < %s -analyze -block-freq | FileCheck %s
; RUN: opt < %s -passes='print<block-freq>' -disable-output 2>&1 | FileCheck %s

define i32 @test1(i32 %i, i32 %a, i32 %b) {
; CHECK-LABEL: Printing analysis {{.*}} for function 'test1':
; CHECK-NEXT: block-frequency-info: test1
; CHECK-NEXT: entry: float = 1.0, int = [[ENTRY:[0-9]+]]
entry:
  %cond = icmp ult i32 %i, 42
  br i1 %cond, label %then, label %else, !prof !0

; The 'then' branch is predicted more likely via branch weight metadata.
; CHECK-NEXT: then: float = 0.9411{{[0-9]*}},
then:
  br label %exit

; CHECK-NEXT: else: float = 0.05882{{[0-9]*}},
else:
  br label %exit

; CHECK-NEXT: exit: float = 1.0, int = [[ENTRY]]
exit:
  %result = phi i32 [ %a, %then ], [ %b, %else ]
  ret i32 %result
}

define i32 @test2(i32 %i, i32 %a, i32 %b) {
; CHECK-LABEL: Printing analysis {{.*}} for function 'test2':
; CHECK-NEXT: block-frequency-info: test2
; CHECK-NEXT: entry: float = 1.0, int = [[ENTRY:[0-9]+]]
entry:
  %cond = icmp ult i32 %i, 42
  br i1 %cond, label %then, label %else, !prof !1

; The 'then' branch is predicted more likely via branch weight metadata.
; CHECK-NEXT: then: float = 0.9411{{[0-9]*}},
then:
  br label %exit

; CHECK-NEXT: else: float = 0.05882{{[0-9]*}},
else:
  br label %exit

; CHECK-NEXT: exit: float = 1.0, int = [[ENTRY]]
exit:
  %result = phi i32 [ %a, %then ], [ %b, %else ]
  ret i32 %result
}

!0 = !{!"branch_weights", i32 64, i32 4}
!1 = !{!1, !2, !3, !4}
!2 = !{!"foo"}
!3 = !{!"branch_weights", i32 64, i32 4}
!4 = !{!"bar"}
