; RUN: opt -simplifycfg -S < %s | FileCheck %s

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
; CHECK: %switch.peeledcmp = icmp eq i32 %conv, 8
; CHECK: br i1 %switch.peeledcmp, label %return, label %peeled.switch, !prof ![[BRANCH_COUNT:[0-9]+]]
; CHECK: peeled.switch:
; CHECK-NOT: i32 8, label %return
; CHECK: ], !prof ![[SWITCH_COUNT:[0-9]+]]


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

!1 = !{!"function_entry_count", i64 101000}
!2 = !{!"branch_weights", i32 0, i32 100000, i32 0, i32 1000, i32 0, i32 0}
; CHECK: ![[BRANCH_COUNT]] = !{!"branch_weights", i32 100000, i32 1000}
; CHECK: ![[SWITCH_COUNT]] = !{!"branch_weights", i32 0, i32 0, i32 1000, i32 0, i32 0}

