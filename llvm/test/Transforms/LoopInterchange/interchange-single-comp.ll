; RUN: opt < %s -loop-interchange -S | FileCheck %s

;;--------------------------------------Test case 01------------------------------------
;; It is profitable to interchange those loops, as both A[j*25+i] and B[n+i]
;; will sequentially access memory.
;;  for(int i=0;i<n;i++)
;;    for(int j=1;j<25;j++)
;;      A[j*25+i] = B[n+i]

define void @foo(i32* %A, i32* %B, i64 %n) {
entry:
  br label %outer.preheader

outer.preheader:                           ; preds = %entry, %outer.for.inc8_crit_edge.us
  %i = phi i64 [ 0, %entry ], [ %i.next, %outer.inc8_crit_edge.us ]
  %0 = add i64 %i, %n
  %arrayidx.us = getelementptr inbounds i32, i32* %B, i64 %0
  br label %inner.body

inner.body:                                     ; preds = %outer.preheader, %inner.body
  %j = phi i64 [ %j.next, %inner.body ], [ 0, %outer.preheader ]
  %1 = load i32, i32* %arrayidx.us, align 4
  %2 = mul i64 %j, 25
  %3 = add i64 %2, %i
  %arrayidx6.us = getelementptr inbounds i32, i32* %A, i64 %3
  %4 = load i32, i32* %arrayidx6.us, align 4
  %add7.us = add i32 %4, %1
  store i32 %add7.us, i32* %arrayidx6.us, align 4
  %j.next = add i64 %j, 1
  %exitcond = icmp ne i64 %j.next, %n
  br i1 %exitcond, label %inner.body, label %outer.inc8_crit_edge.us

outer.inc8_crit_edge.us:                  ; preds = %inner.body
  %i.next = add i64 %i, 1
  %exitcond29 = icmp ne i64 %i.next, 25
  br i1 %exitcond29, label %outer.preheader, label %for.end10.loopexit

for.end10.loopexit:                               ; preds = %outer.inc8_crit_edge.us
  br label %for.end10

for.end10:                                        ; preds = %entry, %for.end10.loopexit
  ret void
}

; CHECK: split
