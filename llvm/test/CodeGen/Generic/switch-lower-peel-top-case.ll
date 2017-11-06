; RUN: llc -stop-after=isel < %s  | FileCheck %s

define i32 @foo(i32 %n) !prof !1 {
entry:
  switch i32 %n, label %bb_default [
    i32 8, label %bb1
    i32 -8826, label %bb2
    i32 18312, label %bb3
    i32 18568, label %bb4
    i32 129, label %bb5
  ], !prof !2

; CHECK: successors: %[[PEELED_CASE_LABEL:.*]](0x5999999a), %[[PEELED_SWITCH_LABEL:.*]](0x26666666)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri %{{[0-9]+}}, 18568, implicit-def %eflags
; CHECK:    JE_1 %[[PEELED_CASE_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[PEELED_SWITCH_LABEL]]
; CHECK:  [[PEELED_SWITCH_LABEL]]:
; CHECK:    successors: %[[BB1_LABEL:.*]](0x0206d3a0), %[[BB2_LABEL:.*]](0x7df92c60)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri %{{[0-9]+}}, 18311, implicit-def %eflags
; CHECK:    JG_1 %[[BB2_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[BB1_LABEL]]
; CHECK:  [[BB1_LABEL]]:
; CHECK:    successors: %[[BB3_LABEL:.*]](0x35e50d5b), %[[BB4_LABEL:.*]](0x4a1af2a5)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri %{{[0-9]+}}, -8826, implicit-def %eflags
; CHECK:    JE_1 %[[BB3_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[BB4_LABEL]]
; CHECK:  [[BB4_LABEL]]
; CHECK:    successors: %[[BB5_LABEL:.*]](0x45d173c8), %[[BB6_LABEL:.*]](0x3a2e8c38)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri %{{[0-9]+}}, 129, implicit-def %eflags
; CHECK:    JE_1 %[[BB5_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[BB6_LABEL]]
; CHECK:  [[BB6_LABEL:.*]]:
; CHECK:    successors: %[[BB7_LABEL:.*]](0x66666666), %[[BB8_LABEL:.*]](0x1999999a)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri8 %{{[0-9]+}}, 8, implicit-def %eflags
; CHECK:    JE_1 %[[BB7_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[BB8_LABEL]]
; CHECK:  [[BB2_LABEL]]:
; CHECK:    successors: %[[BB9_LABEL:.*]](0x7fe44107), %[[BB8_LABEL]](0x001bbef9)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri %{{[0-9]+}}, 18312, implicit-def %eflags
; CHECK:    JE_1 %[[BB9_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[BB8_LABEL]]

bb1:
  br label %return
bb2:
  br label %return
bb3:
  br label %return
bb4:
  br label %return
bb5:
  br label %return
bb_default:
  br label %return

return:
  %retval = phi i32 [ 0, %bb_default ], [ 5, %bb5 ], [ 4, %bb4 ], [ 3, %bb3 ], [ 2, %bb2 ], [ 1, %bb1 ]
  ret i32 %retval
}

define i32 @foo1(i32 %n) !prof !1 {
entry:
  switch i32 %n, label %bb_default [
    i32 1, label %bb1
    i32 86, label %bb2
    i32 85, label %bb2
    i32 3, label %bb3
  ], !prof !3

; CHECK:   successors: %[[RETURN_LABEL:.*]](0x59999999), %[[PEELED_SWITCH_LABEL:.*]](0x26666667)
; CHECK:   %{{[0-9]+}}:gr32 = ADD32ri8 %{{[0-9]+}}, -85, implicit-def dead %eflags
; CHECK:   %{{[0-9]+}}:gr32 = SUB32ri8 %{{[0-9]+}}, 2, implicit-def %eflags
; CHECK:   JB_1 %[[RETURN_LABEL]], implicit %eflags
; CHECK:   JMP_1 %[[PEELED_SWITCH_LABEL]]
; CHECK: [[PEELED_SWITCH_LABEL]]:
; CHECK:   successors: %[[BB1_LABEL:.*]](0x7f5c28f4), %[[BB2_LABEL:.*]](0x00a3d70c)
; CHECK:   %{{[0-9]+}}:gr32 = SUB32ri8 %{{[0-9]+}}, 3, implicit-def %eflags
; CHECK:   JE_1 %[[BB1_LABEL]], implicit %eflags
; CHECK:   JMP_1 %[[BB2_LABEL]]
; CHECK: [[BB2_LABEL]]:
; CHECK:   successors: %[[BB3_LABEL:.*]](0x55555555), %[[DEFAULT_BB_LABEL:.*]](0x2aaaaaab)
; CHECK:   %{{[0-9]+}}:gr32 = SUB32ri8 %{{[0-9]+}}, 1, implicit-def %eflags
; CHECK:   JNE_1 %[[DEFAULT_BB_LABEL]], implicit %eflags
; CHECK:   JMP_1 %[[BB3_LABEL]]

bb1:
  br label %return
bb2:
  br label %return
bb3:
  br label %return
bb_default:
  br label %return

return:
  %retval = phi i32 [ 0, %bb_default ], [ 3, %bb3 ], [ 2, %bb2 ], [ 1, %bb1 ]
  ret i32 %retval
}
!1 = !{!"function_entry_count", i64 100000}
!2 = !{!"branch_weights", i32 50, i32 100, i32 200, i32 29500, i32 70000, i32 150}
!3 = !{!"branch_weights", i32 50, i32 100, i32 500, i32 69500, i32 29850}

