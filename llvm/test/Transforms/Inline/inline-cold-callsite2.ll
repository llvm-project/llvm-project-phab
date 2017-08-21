; RUN: opt < %s -passes='require<profile-summary>,cgscc(inline)' -inline-cold-callsite-threshold=0 -S | FileCheck %s
define void @bar(i32 %a) local_unnamed_addr {
entry:
  tail call void @baz(i32 %a)
  tail call void @baz(i32 %a)
  tail call void @baz(i32 %a)
  ret void
}

declare void @baz(i32) local_unnamed_addr

define void @foo(i32 %a) local_unnamed_addr {
entry:
  %cmp = icmp slt i32 %a, 10
  br i1 %cmp, label %if.then, label %if.end, !prof !2

if.then:                                          ; preds = %entry
  call void @bar(i32 %a)
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

; CHECK-LABEL: @foobar
define void @foobar(i32 %a) local_unnamed_addr {
entry:
  %mul = shl nsw i32 %a, 1
; CHECK-NOT: call void @foo
; CHECK: call void @bar
  call void @foo(i32 %mul)
  ret void
}

!2 = !{!"branch_weights", i32 1, i32 2000}
