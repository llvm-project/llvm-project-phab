; RUN: opt < %s -jump-threading -S | FileCheck %s

; This test examines how jump-threading handles branch weights metadata
; if the branch to be eliminated has the metadata.

define i32 @func(i32 %flag) {
; CHECK-LABEL: @func
; CHECK: br i1 %tobool.i, label %if.hot, label %if.cold, !prof !0
; CHECK-DAG: if.hot:
; CHECK-DAG: if.cold:
entry:
  %tobool.i = icmp eq i32 %flag, 0
  br i1 %tobool.i, label %to.be.eliminated, label %if.then

if.then:
  br label %to.be.eliminated

to.be.eliminated:
  %retval.0.i = phi i32 [ 0, %if.then ], [ 1, %entry ]
  %cmp = icmp eq i32 %retval.0.i, 0
  br i1 %cmp, label %if.cold, label %if.hot, !prof !1

if.cold:
  call void @cold_func()
  br label %return

if.hot:
  call void @hot_func()
  br label %return

return:
  %retval.0 = phi i32 [ 0, %if.cold ], [ 1, %if.hot ]
  ret i32 %retval.0
}

define i32 @func2(i32 %flag) {
; CHECK-LABEL: @func2
; CHECK: br i1 %tobool.i, label %if.cold, label %if.hot, !prof !1
; CHECK-DAG: if.hot:
; CHECK-DAG: if.cold:
entry:
  %tobool.i = icmp eq i32 %flag, 0
  br i1 %tobool.i, label %to.be.eliminated, label %if.then

if.then:
  br label %to.be.eliminated

to.be.eliminated:
  %retval.0.i = phi i32 [ 0, %if.then ], [ 1, %entry ]
  %cmp = icmp eq i32 %retval.0.i, 0
  br i1 %cmp, label %if.hot, label %if.cold, !prof !0

if.cold:
  call void @cold_func()
  br label %return

if.hot:
  call void @hot_func()
  br label %return

return:
  %retval.0 = phi i32 [ 0, %if.cold ], [ 1, %if.hot ]
  ret i32 %retval.0
}


define signext i32 @func3(i32 signext %flag) {
; This is a test with more complicated CFG.
; The following is the equivalent C code.
; We want to set branch weights for "if (flag & 1)" and "if (flag & 2)".
;
; inline int bar(int flag) {
; 	if (flag & 1) {
; 		cold_func();
; 		return 0;
; 	}
; 	if (flag & 2) {
; 		if (__builtin_expect(flag & 4, 0)) cold_func();
; 		cold_func();
; 		return 0;
; 	}
; 	if (__builtin_expect(flag & 8, 0)) cold_func();
; 	return 1;
; }
;
; int func3(int flag) {
; 	if (__builtin_expect(0 == bar(flag), 0)) {
; 		cold_func();
; 		return 0;
; 	}
; 	hot_func();
; 	return 1;
; }

; CHECK-LABEL: @func3
; CHECK: br i1 %tobool.i, label %if.end.i, label %if.then.i, !prof !0
; CHECK: if.then.i:
; CHECK: br i1 %tobool2.i, label %if.end8.i, label %if.then3.i, !prof !0
; CHECK: if.then3.i:

entry:
  %and.i = and i32 %flag, 1
  %tobool.i = icmp eq i32 %and.i, 0
  br i1 %tobool.i, label %if.end.i, label %if.then.i

if.then.i:
  call void @cold_func()
  br label %bar.exit

if.end.i:
  %and1.i = and i32 %flag, 2
  %tobool2.i = icmp eq i32 %and1.i, 0
  br i1 %tobool2.i, label %if.end8.i, label %if.then3.i

if.then3.i:
  %and4.i = and i32 %flag, 4
  %tobool5.i = icmp eq i32 %and4.i, 0
  br i1 %tobool5.i, label %if.end7.i, label %if.then6.i, !prof !0

if.then6.i:
  call void @cold_func()
  br label %if.end7.i

if.end7.i:
  call void @cold_func()
  br label %bar.exit

if.end8.i:
  %and9.i = and i32 %flag, 8
  %tobool12.i = icmp eq i32 %and9.i, 0
  br i1 %tobool12.i, label %bar.exit, label %if.then13.i, !prof !0

if.then13.i:
  call void @cold_func()
  br label %bar.exit

bar.exit:
  %retval.0.i = phi i32 [ 0, %if.then.i ], [ 0, %if.end7.i ], [ 1, %if.end8.i ], [ 1, %if.then13.i ]
  %cmp = icmp eq i32 %retval.0.i, 0
  br i1 %cmp, label %if.then, label %if.end, !prof !1

if.then:
  call void @cold_func()
  br label %return

if.end:
  call void @hot_func()
  br label %return

return:
  %retval.0 = phi i32 [ 0, %if.then ], [ 1, %if.end ]
  ret i32 %retval.0
}

; CHECK-DAG: !0 = !{!"branch_weights", i32 2000, i32 1}
; CHECK-DAG: !1 = !{!"branch_weights", i32 1, i32 2000}


declare void @hot_func()
declare void @cold_func()
!0 = !{!"branch_weights", i32 2000, i32 1}
!1 = !{!"branch_weights", i32 1, i32 2000}
