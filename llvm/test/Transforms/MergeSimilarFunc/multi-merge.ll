; RUN: opt -mergesimilarfunc -mergesimilarfunc-level=all < %s -S | FileCheck %s

; CHECK: define i32 @A
; CHECK: define internal i32 @A__merged
; CHECK: define i32 @B
; CHECK: call i32 @A__merged
; CHECK: define i32 @C
; CHECK: call i32 @A__merged
; CHECK: define i32 @D
; CHECK: call i32 @A__merged

define i32 @A(i32* %a, i32 %n, i32 %c) nounwind {
entry:
  %0 = bitcast i32* %a to i8*
  %mul = mul i32 %n, 4
  %call = tail call i8* @memset1(i8* %0, i32 %c, i32 %mul) nounwind
  %cmp7 = icmp sgt i32 %n, 0
  br i1 %cmp7, label %for.body, label %for.end

for.body:
  %arrayidx.phi = phi i32* [ %arrayidx.inc, %for.body ], [ %a, %entry ]
  %i.09 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %sum.08 = phi i32 [ %add, %for.body ], [ 0, %entry ]
  %1 = load i32, i32* %arrayidx.phi, align 4
  %add = add nsw i32 %1, %sum.08
  %inc = add nsw i32 %i.09, 1
  %exitcond = icmp eq i32 %inc, %n
  %arrayidx.inc = getelementptr i32, i32* %arrayidx.phi, i32 1
  br i1 %exitcond, label %for.cond.for.end_crit_edge, label %for.body

for.cond.for.end_crit_edge:
  %phitmp = add i32 %add, 1
  br label %for.end

for.end:
  %sum.0.lcssa = phi i32 [ %phitmp, %for.cond.for.end_crit_edge ], [ 1, %entry ]
  ret i32 %sum.0.lcssa
}

declare i8* @memset1(i8*, i32, i32)

define i32 @B(i32* %a, i32 %n, i32 %c) nounwind {
entry:
  %0 = bitcast i32* %a to i8*
  %mul = mul i32 %n, 4
  %call = tail call i8* @memset1(i8* %0, i32 %c, i32 %mul) nounwind
  %cmp7 = icmp sgt i32 %n, 0
  br i1 %cmp7, label %for.body, label %for.end

for.body:
  %arrayidx.phi = phi i32* [ %arrayidx.inc, %for.body ], [ %a, %entry ]
  %i.09 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %sum.08 = phi i32 [ %add, %for.body ], [ 0, %entry ]
  %1 = load i32, i32* %arrayidx.phi, align 4
  %add = add nsw i32 %1, %sum.08
  %inc = add nsw i32 %i.09, 1
  %exitcond = icmp eq i32 %inc, %n
  %arrayidx.inc = getelementptr i32, i32* %arrayidx.phi, i32 1
  br i1 %exitcond, label %for.cond.for.end_crit_edge, label %for.body

for.cond.for.end_crit_edge:
  %phitmp = add i32 %add, 2
  br label %for.end

for.end:
  %sum.0.lcssa = phi i32 [ %phitmp, %for.cond.for.end_crit_edge ], [ 2, %entry ]
  ret i32 %sum.0.lcssa
}

define i32 @C(i32* %a, i32 %n, i32 %c) nounwind {
entry:
  %0 = bitcast i32* %a to i8*
  %mul = mul i32 %n, 4
  %call = tail call i8* @memset1(i8* %0, i32 %c, i32 %mul) nounwind
  %cmp7 = icmp sgt i32 %n, 0
  br i1 %cmp7, label %for.body, label %for.end

for.body:
  %arrayidx.phi = phi i32* [ %arrayidx.inc, %for.body ], [ %a, %entry ]
  %i.09 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %sum.08 = phi i32 [ %add, %for.body ], [ 1, %entry ]
  %1 = load i32, i32* %arrayidx.phi, align 4
  %add = add nsw i32 %1, %sum.08
  %inc = add nsw i32 %i.09, 1
  %exitcond = icmp eq i32 %inc, %n
  %arrayidx.inc = getelementptr i32, i32* %arrayidx.phi, i32 1
  br i1 %exitcond, label %for.cond.for.end_crit_edge, label %for.body

for.cond.for.end_crit_edge:
  %phitmp = add i32 %add, 3
  br label %for.end

for.end:
  %sum.0.lcssa = phi i32 [ %phitmp, %for.cond.for.end_crit_edge ], [ 4, %entry ]
  ret i32 %sum.0.lcssa
}

define i32 @D(i32* %a, i32 %n, i32 %c) nounwind {
entry:
  %0 = bitcast i32* %a to i8*
  %mul = mul i32 %n, 4
  %call = tail call i8* @memset1(i8* %0, i32 %c, i32 %mul) nounwind
  %cmp7 = icmp sgt i32 %n, 0
  br i1 %cmp7, label %for.body, label %for.end

for.body:
  %arrayidx.phi = phi i32* [ %arrayidx.inc, %for.body ], [ %a, %entry ]
  %i.09 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %sum.08 = phi i32 [ %add, %for.body ], [ 1, %entry ]
  %1 = load i32, i32* %arrayidx.phi, align 4
  %add = add nsw i32 %1, %sum.08
  %inc = add nsw i32 %i.09, 1
  %exitcond = icmp eq i32 %inc, %n
  %arrayidx.inc = getelementptr i32, i32* %arrayidx.phi, i32 1
  br i1 %exitcond, label %for.cond.for.end_crit_edge, label %for.body

for.cond.for.end_crit_edge:
  %phitmp = add i32 %add, 4
  br label %for.end

for.end:
  %sum.0.lcssa = phi i32 [ %phitmp, %for.cond.for.end_crit_edge ], [ 5, %entry ]
  ret i32 %sum.0.lcssa
}
