; RUN: llc -mcpu=corei7 -mtriple=x86_64-linux < %s | FileCheck %s --check-prefix=CHECK --check-prefix=NOPRECISE
; RUN: llc -mcpu=corei7 -mtriple=x86_64-linux -force-evict-cold-blocks-from-loops -force-loop-cold-block < %s | FileCheck %s --check-prefix=CHECK --check-prefix=PRECISE

define void @foo() !prof !1 {
; Test if a cold block in a loop will be placed at the end of the function
; chain.
;
; CHECK-LABEL: foo:
; CHECK: callq b
; CHECK: callq c
; CHECK: callq e
; CHECK: callq f
; CHECK: callq d

entry:
  br label %header

header:
  call void @b()
  %call = call zeroext i1 @a()
  br i1 %call, label %if.then, label %if.else, !prof !4

if.then:
  call void @c()
  br label %if.end

if.else:
  call void @d()
  br label %if.end

if.end:
  call void @e()
  %call2 = call zeroext i1 @a()
  br i1 %call2, label %header, label %end, !prof !5

end:
  call void @f()
  ret void
}

define void @nested_loop_0(i1 %flag) !prof !1 {
; Test if a block that is cold in the inner loop but not cold in the outer loop
; will merged to the outer loop chain.
;
; CHECK-LABEL: nested_loop_0:
; PRECISE: callq b
; CHECK: callq c
; CHECK: callq d
; CHECK: callq e
; NOPRECISE: callq b
; CHECK: callq f

entry:
  br label %header

header:
  call void @b()
  %call4 = call zeroext i1 @a()
  br i1 %call4, label %header2, label %end

header2:
  call void @c()
  %call = call zeroext i1 @a()
  br i1 %call, label %if.then, label %if.else, !prof !2

if.then:
  call void @d()
  %call3 = call zeroext i1 @a()
  br i1 %call3, label %header2, label %header, !prof !3

if.else:
  call void @e()
  br i1 %flag, label %header2, label %header, !prof !3

end:
  call void @f()
  ret void
}

define void @nested_loop_1() !prof !1 {
; Test if a cold block in an inner loop will be placed at the end of the
; function chain.
;
; CHECK-LABEL: nested_loop_1:
; CHECK: callq b
; CHECK: callq c
; CHECK: callq e
; CHECK: callq d

entry:
  br label %header

header:
  call void @b()
  br label %header2

header2:
  call void @c()
  %call = call zeroext i1 @a()
  br i1 %call, label %end, label %if.else, !prof !4

if.else:
  call void @d()
  %call2 = call zeroext i1 @a()
  br i1 %call2, label %header2, label %header, !prof !5

end:
  call void @e()
  ret void
}

define void @cold_block_in_hot_loop() !prof !1 {
; Test that a cold block gets moved out of a high-trip-count loop.
;
; CHECK-LABEL: cold_block_in_hot_loop
; NOPRECISE: callq b
; NOPRECISE: callq d
; NOPRECISE: callq e
; NOPRECISE: callq c
; With precise rotation cost, the %header -> %cold edge (10 occurrences)
; is preferred over the %entry -> %header edge (1 occurrence).
; PRECISE: callq d
; PRECISE: callq b
; PRECISE: callq c
; PRECISE: callq e
entry:
  br label %header

header:
  call void @b()
  %call = call i1 @a()
  br i1 %call, label %cold, label %after.cold, !prof !7

cold:
  call void @c()
  br label %after.cold

after.cold:
  call void @d()
  %cont = call i1 @a()
  br i1 %cont, label %header, label %done, !prof !8

done:
  call void @e()
  ret void
}

define void @no_profile_data() {
; Test that we sensibly place a cold block within a loop even when no
; overall profile data is available, when precise loop rotation cost
; and cold loop block eviction are enabled.
;
; CHECK-LABEL: no_profile_data:
; NOPRECISE: callq a
; NOPRECISE: callq d
; NOPRECISE: callq g
; NOPRECISE: callq b
; NOPRECISE: callq c
; PRECISE: callq a
; PRECISE: callq c
; PRECISE: callq g
; PRECISE: callq b
; PRECISE: callq d
entry:
  br label %for.outer.body

for.outer.body:
  %i = phi i32 [ 0, %entry ], [ %i.inc, %for.outer.inc ]
  %br = call i1 @a()
  br label %for.inner.body

for.inner.body:
  %i.inner = phi i32 [ 0, %for.outer.body ], [ %i.inc, %for.inner.inc ]
  br i1 %br, label %if.then, label %if.else

if.then:
  %br.unlikely = call i1 @g()
  br i1 %br.unlikely, label %unlikely, label %if.end, !prof !6

unlikely:
  call void @d()
  br label %if.end

if.end:
  call void @b()
  br label %for.inner.inc

if.else:
  call void @c()
  br label %for.inner.inc

for.inner.inc:
  %i.inc = add i32 %i.inner, 1
  %done.inner = icmp eq i32 %i.inc, 1000
  br i1 %done.inner, label %for.outer.inc, label %for.inner.body

for.outer.inc:
  %done.outer = icmp eq i32 %i.inc, 1000000
  br i1 %done.outer, label %ret, label %for.outer.body

ret:
  ret void
}

declare zeroext i1 @a()
declare void @b()
declare void @c()
declare void @d()
declare void @e()
declare void @f()
declare zeroext i1 @g()

!1 = !{!"function_entry_count", i64 1}
!2 = !{!"branch_weights", i32 100, i32 1}
!3 = !{!"branch_weights", i32 1, i32 10}
!4 = !{!"branch_weights", i32 1000, i32 1}
!5 = !{!"branch_weights", i32 100, i32 1}
!6 = !{!"branch_weights", i32 1, i32 2000}
!7 = !{!"branch_weights", i32 1, i32 100000}
!8 = !{!"branch_weights", i32 1000000, i32 1}
