; REQUIRES: asserts
; RUN: opt < %s -loop-rotate -stats 2>&1 | FileCheck %s

; PR7447: there should be two loops rotated.
; CHECK: 2 loop-rotate

define i32 @test1([100 x i32]* nocapture %a) nounwind readonly {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.cond1, %entry
  %sum.0 = phi i32 [ 0, %entry ], [ %sum.1, %for.cond1 ]
  %i.0 = phi i1 [ true, %entry ], [ false, %for.cond1 ]
  br i1 %i.0, label %for.cond1, label %return

for.cond1:                                        ; preds = %for.cond, %land.rhs
  %sum.1 = phi i32 [ %add, %land.rhs ], [ %sum.0, %for.cond ]
  %i.1 = phi i32 [ %inc, %land.rhs ], [ 0, %for.cond ]
  %cmp2 = icmp ult i32 %i.1, 100
  br i1 %cmp2, label %land.rhs, label %for.cond

land.rhs:                                         ; preds = %for.cond1
  %conv = zext i32 %i.1 to i64
  %arrayidx = getelementptr inbounds [100 x i32], [100 x i32]* %a, i64 0, i64 %conv
  %0 = load i32, i32* %arrayidx, align 4
  %add = add i32 %0, %sum.1
  %cmp4 = icmp ugt i32 %add, 1000
  %inc = add i32 %i.1, 1
  br i1 %cmp4, label %return, label %for.cond1

return:                                           ; preds = %for.cond, %land.rhs
  %retval.0 = phi i32 [ 1000, %land.rhs ], [ %sum.0, %for.cond ]
  ret i32 %retval.0
}
