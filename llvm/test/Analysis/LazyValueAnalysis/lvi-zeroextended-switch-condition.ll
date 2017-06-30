; RUN: opt < %s -jump-threading -print-lazy-value-info -disable-output 2>&1 | FileCheck %s

declare void @bar(i32)

define i32 @foo_switch(i8* nocapture readonly %p, i32 (i32)* nocapture %f) #0 {
entry:
  %targets = alloca [256 x i8*], align 16
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %cmp = icmp slt i32 %i.0, 256
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %idxprom = sext i32 %i.0 to i64
  %arrayidx = getelementptr inbounds [256 x i8*], [256 x i8*]* %targets, i64 0, i64 %idxprom
  store i8* blockaddress(@foo_switch, %sw.default), i8** %arrayidx, align 8
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %arrayidx1 = getelementptr inbounds [256 x i8*], [256 x i8*]* %targets, i64 0, i64 93
  store i8* blockaddress(@foo_switch, %if.end), i8** %arrayidx1, align 8
  %arrayidx2 = getelementptr inbounds [256 x i8*], [256 x i8*]* %targets, i64 0, i64 145
  store i8* blockaddress(@foo_switch, %dispatch_op), i8** %arrayidx2, align 8
  br label %for.cond3

for.cond3:                                        ; preds = %sw.default, %for.end
  %p.addr.0 = phi i8* [ %p, %for.end ], [ %p.addr.4, %sw.default ]
  br label %dispatch_op

dispatch_op:                                      ; preds = %dispatch_op, %if.end, %for.cond3
  %p.addr.0.pn = phi i8* [ %p.addr.0, %for.cond3 ], [ %p.addr.1, %dispatch_op ], [ %incdec.ptr6, %if.end ]
  %op.0.in = load i8, i8* %p.addr.0.pn, align 1
  %p.addr.1 = getelementptr inbounds i8, i8* %p.addr.0.pn, i64 1
  %op.0 = zext i8 %op.0.in to i32
  switch i8 %op.0.in, label %sw.default [
    i8 93, label %if.end
    i8 -111, label %dispatch_op
  ]

; Check that lazy value info propagates the zero-extended switch condition value through the case
; edge and figures that %op.1 is 93.
;
; CHECK-LABEL: if.end:
; CHECK:       ; LatticeVal for: '  %op.1 = phi i32 [ %op.0, %dispatch_op ], [ 93, %if.end ]' in BB: '%if.end' is: constantrange<93, 94>
if.end:                                           ; preds = %dispatch_op, %if.end
  %p.addr.2 = phi i8* [ %p.addr.1, %dispatch_op ], [ %incdec.ptr6, %if.end ]
  %op.1 = phi i32 [ %op.0, %dispatch_op ], [ 93, %if.end ]
  %incdec.ptr6 = getelementptr inbounds i8, i8* %p.addr.2, i64 1
  %0 = load i8, i8* %p.addr.2, align 1
  %idxprom7 = zext i8 %0 to i64
  %arrayidx8 = getelementptr inbounds [256 x i8*], [256 x i8*]* %targets, i64 0, i64 %idxprom7
  %1 = load i8*, i8** %arrayidx8, align 8
  indirectbr i8* %1, [label %sw.default, label %if.end, label %dispatch_op]

sw.default:                                       ; preds = %if.end, %dispatch_op
  %p.addr.4 = phi i8* [ %p.addr.1, %dispatch_op ], [ %incdec.ptr6, %if.end ]
  %op.2 = phi i32 [ %op.0, %dispatch_op ], [ %op.1, %if.end ]
  call void @bar(i32 %op.2) #2
  br label %for.cond3
}
