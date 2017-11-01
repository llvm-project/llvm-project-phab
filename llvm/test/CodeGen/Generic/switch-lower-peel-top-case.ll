; RUN: llc -stop-after=isel < %s  | FileCheck %s

define i32 @foo(i16 signext %n) local_unnamed_addr !prof !1 {
entry:
  %conv = sext i16 %n to i32
  switch i32 %conv, label %sw.epilog [
    i32 8, label %return
    i32 -8826, label %sw.bb1
    i32 18312, label %sw.bb3
    i32 18568, label %sw.bb5
    i32 129, label %sw.bb7
  ], !prof !2

; CHECK: successors: %[[BB5_LABEL:.*]](0x5999999a), %[[PeeledSwitch_LABEL:.*]](0x26666666)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri %{{[0-9]+}}, 18568, implicit-def %eflags
; CHECK:    JE_1 %[[BB5_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[PeeledSwitch_LABEL]]
; CHECK:  [[PeeledSwitch_LABEL]]:
; CHECK:    successors: %[[BB8_LABEL:.*]](0x0206d3a0), %[[BB9_LABEL:.*]](0x7df92c60)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri %{{[0-9]+}}, 18311, implicit-def %eflags
; CHECK:    JG_1 %[[BB9_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[BB8_LABEL]]
; CHECK:  [[BB8_LABEL]]:
; CHECK:    successors: %[[BB1_LABEL:.*]](0x35e50d5b), %[[BB10_LABEL:.*]](0x4a1af2a5)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri %{{[0-9]+}}, -8826, implicit-def %eflags
; CHECK:    JE_1 %[[BB1_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[BB10_LABEL]]
; CHECK:  [[BB10_LABEL]]
; CHECK:    successors: %[[BB7_LABEL:.*]](0x45d173c8), %[[BB11_LABEL:.*]](0x3a2e8c38)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri %{{[0-9]+}}, 129, implicit-def %eflags
; CHECK:    JE_1 %[[BB7_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[BB11_LABEL]]
; CHECK:  [[BB11_LABEL:.*]]:
; CHECK:    successors: %[[BB6_LABEL:.*]](0x66666666), %[[BB51_LABEL:.*]](0x1999999a)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri8 %{{[0-9]+}}, 8, implicit-def %eflags
; CHECK:    JE_1 %[[BB6_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[BB51_LABEL]]
; CHECK:  [[BB9_LABEL]]:
; CHECK:    successors: %[[BB2_LABEL:.*]](0x7fe44107), %[[BB51_LABEL]](0x001bbef9)
; CHECK:    %{{[0-9]+}}:gr32 = SUB32ri %{{[0-9]+}}, 18312, implicit-def %eflags
; CHECK:    JE_1 %[[BB2_LABEL]], implicit %eflags
; CHECK:    JMP_1 %[[BB51_LABEL]]

sw.bb1:
  br label %return

sw.bb3:
  br label %return

sw.bb5:
  br label %return

sw.bb7:
  br label %return

sw.epilog:
  br label %return

return:
  %retval = phi i32 [ 0, %sw.epilog ], [ 5, %sw.bb7 ], [ 4, %sw.bb5 ], [ 3, %sw.bb3 ], [ 2, %sw.bb1 ], [ 1, %entry ]
  ret i32 %retval
}

!1 = !{!"function_entry_count", i64 100000}
!2 = !{!"branch_weights", i32 50, i32 100, i32 200, i32 29500, i32 70000, i32 150}

