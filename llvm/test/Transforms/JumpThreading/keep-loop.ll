; RUN: opt < %s -jump-threading -loop-unswitch -S | FileCheck %s

; This tests jump threading do not turn a proper loop structures into an
; irreducible loop by SimplifyPartiallyRedundantLoad(). This test contain
; obvious unswithch opportunity which should be handled by loop unswitch pass.
;
; CHECK-LABEL: @fn_keep_loop
; CHECK-LABEL: while.cond2.us:
; CHECK: br i1 true, label %if.then.us, label %if.end.us
; CHECK-LABELif.then.us:
; CHECK: call void @fn3(i64 %l3.us)
; CHECK-LABEL: while.cond2:
; CHECK: br i1 false, label %if.then, label %if.end
; CHECK-LABEL: if.then:
; CHECK: call void @fn3(i64 %l3)

define i32 @fn_keep_loop(i1 %c0, i1 %c1, i64* %ptr0, i64* %ptr1) {
entry:
  br i1 %c0, label %while.cond, label %sw.epilog

while.cond:                                       ; preds = %land.rhs, %entry
  %magicptr = load i64, i64* %ptr1, align 8
  %c = icmp eq i64 %magicptr, 0
  br i1 %c, label %while.cond2, label %land.rhs

land.rhs:                                         ; preds = %while.cond
  %arrayidx = getelementptr inbounds i64, i64* %ptr0, i64 1
  %l2 = load i64, i64* %arrayidx, align 4
  %tobool1 = icmp eq i64 %l2, 0
  br i1 %tobool1, label %while.cond2, label %while.cond

while.cond2:
  %l3 = load i64, i64* %ptr1, align 8
  br i1 %c1, label %if.then, label %if.end

if.then:
  call void @fn3(i64 %l3)
  br label %if.end

if.end:
  %c2 = icmp eq i64 %l3,  0
  br i1 %c2, label %sw.epilog, label %while.body4

while.body4:                                      ; preds = %while.cond2
  call void @fn2(i64 %l3)
  br label %while.cond2

sw.epilog:                                        ; preds = %while.cond2, %entry
  ret i32 undef
}

declare void @fn2(i64)
declare void @fn3(i64)
