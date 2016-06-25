; RUN: opt < %s -S -unroll-runtime -loop-unroll -unroll-max-count=2 | FileCheck %s
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"

; The test checks unroll for uncountable loop.

; CHECK: if.then:
; CHECK: if.then.1:

%struct.list = type { %struct.list*, i32 }

@head = global %struct.list { %struct.list* null, i32 1 }, align 8

; Function Attrs: norecurse nounwind uwtable
define i32 @main(i32 %argc, i8** nocapture readnone %argv) {
entry:
  %0 = load i32, i32* getelementptr inbounds (%struct.list, %struct.list* @head, i64 0, i32 1), align 8
  br label %if.then

if.then:                                          ; preds = %entry, %if.then.for.cond_crit_edge
  %prev.024 = phi %struct.list* [ @head, %entry ], [ %cur.023, %if.then.for.cond_crit_edge ]
  %cur.023 = phi %struct.list* [ @head, %entry ], [ %2, %if.then.for.cond_crit_edge ]
  %1 = phi i32 [ %0, %entry ], [ %.pre, %if.then.for.cond_crit_edge ]
  %next = getelementptr inbounds %struct.list, %struct.list* %cur.023, i64 0, i32 0
  %2 = load %struct.list*, %struct.list** %next, align 8
  %tobool = icmp eq %struct.list* %2, null
  br i1 %tobool, label %if.else, label %if.then.for.cond_crit_edge

if.then.for.cond_crit_edge:                       ; preds = %if.then
  %val.phi.trans.insert = getelementptr inbounds %struct.list, %struct.list* %cur.023, i64 0, i32 1
  %.pre = load i32, i32* %val.phi.trans.insert, align 8
  %cmp = icmp sgt i32 %.pre, %0
  br i1 %cmp, label %for.end.loopexit, label %if.then

if.else:                                          ; preds = %if.then
  %next3 = getelementptr inbounds %struct.list, %struct.list* %prev.024, i64 0, i32 0
  store %struct.list* null, %struct.list** %next3, align 8
  br label %for.end

for.end.loopexit:                                 ; preds = %if.then.for.cond_crit_edge
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %if.else
  %3 = phi i32 [ %1, %if.else ], [ %.pre, %for.end.loopexit ]
  ret i32 %3
}
