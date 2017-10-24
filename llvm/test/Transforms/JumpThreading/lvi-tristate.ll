; RUN: opt -jump-threading -simplifycfg -S < %s | FileCheck %s
; CHECK-NOT: BB2:
; CHECK-NOT: BB3:
; CHECK-NOT: BB4:
; CHECK-NOT: BB6:
; CHECK-NOT: BB7:
; CHECK: entry:
; CHECK: BB0:
; CHECK: BB1:
; CHECK: BB5:
; CHECK: exit:

declare void @foo() local_unnamed_addr

define fastcc void @test() unnamed_addr {
entry:
  %0 = and i32 undef, 1073741823
  %1 = icmp eq i32 %0, 2
  br i1 %1, label %BB7, label %BB0

BB0:
  %2 = icmp eq i32 %0, 3
  br i1 %2, label %exit, label %BB1

BB1:
  %3 = icmp eq i32 %0, 5
  br i1 %3, label %BB2, label %BB3

BB2:
  tail call void @foo()
  br label %BB3

BB3:
  br i1 %2, label %exit, label %BB4

BB4:
  %4 = icmp eq i32 %0, 4
  br i1 %4, label %exit, label %BB5

BB5:
  br i1 %4, label %BB6, label %exit

BB6:
  br label %exit

BB7:
  br label %BB0

exit:
  ret void
}
