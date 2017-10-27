; RUN: llc -debug-only=isel < %s  -o /dev/null 2>&1 | FileCheck %s

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
; CHECK: Peeled one top case in switch stmt, prob: 0x5999999a / 0x80000000 = 70.00%
; CHECK: Scale the probablity for one cluster, before scaling: 0x0020c49c / 0x80000000 = 0.10%
; CHECK: After scaling: 0x006d3a08 / 0x80000000 = 0.33%
; CHECK: Scale the probablity for one cluster, before scaling: 0x00418937 / 0x80000000 = 0.20%
; CHECK: After scaling: 0x00da740d / 0x80000000 = 0.67%
; CHECK: Scale the probablity for one cluster, before scaling: 0x25c28f5c / 0x80000000 = 29.50%
; CHECK: After scaling: 0x7ddddddf / 0x80000000 = 98.33%
; CHECK: Scale the probablity for one cluster, before scaling: 0x003126e9 / 0x80000000 = 0.15%
; CHECK: After scaling: 0x00a3d709 / 0x80000000 = 0.50%
; CHECK: Case clusters: -8826 8 129 18312

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

