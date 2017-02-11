; RUN: opt < %s -S -unroll-runtime -loop-unroll -unroll-allow-force | FileCheck %s
; The test checks unroll for uncountable loop, when previous value is stored.

; CHECK: loop.body.if:
; CHECK: loop.body.if.1:

%struct.list = type { %struct.list*, i32 }

@head = global %struct.list { %struct.list* null, i32 1 }, align 8

; Function Attrs: norecurse nounwind uwtable
define i32 @main(i32 %argc, i8** nocapture readnone %argv) {
entry:
  %0 = load i32, i32* getelementptr inbounds (%struct.list, %struct.list* @head, i64 0, i32 1), align 8
  br label %loop.body

loop.body:                                          ; preds = %entry, %loop.body.if
  %prev = phi %struct.list* [ @head, %entry ], [ %cur, %loop.body.if ]
  %cur = phi %struct.list* [ @head, %entry ], [ %2, %loop.body.if ]
  %1 = phi i32 [ %0, %entry ], [ %3, %loop.body.if ]
  %next.cur = getelementptr inbounds %struct.list, %struct.list* %cur, i64 0, i32 0
  %2 = load %struct.list*, %struct.list** %next.cur, align 8
  %tobool = icmp eq %struct.list* %2, null
  br i1 %tobool, label %if.else, label %loop.body.if

loop.body.if:                       ; preds = %loop.body
  %val = getelementptr inbounds %struct.list, %struct.list* %cur, i64 0, i32 1
  %3 = load i32, i32* %val, align 8
  %cmp = icmp sgt i32 %3, %0
  br i1 %cmp, label %loopexit, label %loop.body

if.else:                                          ; preds = %if.then
  %next.prev = getelementptr inbounds %struct.list, %struct.list* %prev, i64 0, i32 0
  store %struct.list* null, %struct.list** %next.prev, align 8
  br label %end

loopexit:                                 ; preds = %loop.body.if
  br label %end

end:                                          ; preds = %loopexit, %if.else
  %4 = phi i32 [ %1, %if.else ], [ %3, %loopexit ]
  ret i32 %4
}

; The test checks unroll for uncountable loop, when XOR is simplified.

; CHECK: while.body:
; CHECK: while.body.1:

; Function Attrs: norecurse nounwind uwtable
define void @xor(i32* nocapture %a) local_unnamed_addr #0 {
entry:
  %0 = load i32, i32* %a, align 4
  %entry.cmp = icmp eq i32 %0, -1
  br i1 %entry.cmp, label %while.end, label %while.body

while.body:                                       ; preds = %entry, %while.body
  %1 = phi i32 [ %2, %while.body ], [ %0, %entry ]
  %arrayidx12 = phi i32* [ %arrayidx, %while.body ], [ %a, %entry ]
  %s = phi i32 [ %xor, %while.body ], [ 0, %entry ]
  %i = phi i32 [ %inc, %while.body ], [ undef, %entry ]
  %add = add nsw i32 %1, %s
  store i32 %add, i32* %arrayidx12, align 4
  %xor = xor i32 %s, 1
  %inc = add nsw i32 %i, 1
  %arrayidx = getelementptr inbounds i32, i32* %a, i32 %inc
  %2 = load i32, i32* %arrayidx, align 4
  %cmp = icmp eq i32 %2, -1
  br i1 %cmp, label %while.end, label %while.body

while.end:                                        ; preds = %while.body, %entry
  ret void
}

