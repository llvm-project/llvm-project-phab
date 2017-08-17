; RUN: opt < %s -loop-vectorize -disable-output -debug-only=loop-vectorize 2>&1 | FileCheck %s --check-prefix=COST

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"


@a = global i32* null, align 8
@b = global i32* null, align 8
@c = global i32* null, align 8

; This test checks that the frequency of the predicated block(for computing the cost)
; is obtained from BlockFrequencyInfo Class
; COST-LABEL: foo
; COST:       LV: Found an estimated cost of 4 for VF 1 For BasicBlock: if.then3.i


define void @foo(i32 %p1) {
entry:
  %cmp88 = icmp sgt i32 %p1, 0
  br i1 %cmp88, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  %0 = load i32*, i32** @b, align 8
  %1 = load i32*, i32** @a, align 8
  %2 = load i32*, i32** @c, align 8
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %if.end3.i ]
  %arrayidx = getelementptr inbounds i32, i32* %0, i64 %indvars.iv
  %3 = load i32, i32* %arrayidx, align 4  %4 = trunc i64 %indvars.iv to i32
  %and.i = and i32 %4, 1
  %tobool.i.i = icmp eq i32 %and.i, 0
  br i1 %tobool.i.i, label %if.end.i, label %if.then.i

if.then.i:
  %and.i.i = lshr i32 %3, 2
  %cmp.i = icmp sgt i32 %and.i.i, 0
  %conv.i = zext i1 %cmp.i to i32
  br label %if.end.i

if.end.i:
  %p1.addr.0.i = phi i32 [ %conv.i, %if.then.i ], [ %3, %for.body ]
  %5 = trunc i64 %indvars.iv to i32
  %and1.i = and i32 %5, 7
  %tobool2.i = icmp eq i32 %and1.i, 0
  br i1 %tobool2.i, label  %if.end3.i, label %if.then3.i

if.then3.i:
  %p1.addr.0.lobit.i = lshr i32 %p1.addr.0.i, 31
  %and6.i = and i32 %p1.addr.0.i, 1
  %or.i = or i32 %p1.addr.0.lobit.i, %and6.i
  br label %if.end3.i

if.end3.i:
%p1.addr.1.i = phi i32 [ %or.i, %if.then3.i ], [ %p1.addr.0.i, %if.end.i ]

  %6 = trunc i64 %indvars.iv to i32
  %shr.i = ashr i32 %6, 1
  %arrayidx7 = getelementptr inbounds i32, i32* %2, i64 %indvars.iv
  store i32 %p1.addr.1.i, i32* %arrayidx7, align 4  %indvars.iv.next = add i64 %indvars.iv, 1
  %lftr.wideiv = trunc i64 %indvars.iv.next to i32
  %exitcond = icmp ne i32 %lftr.wideiv, %p1
  br i1 %exitcond, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:
  br label %for.end

for.end:
  ret void
}
