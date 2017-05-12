; RUN: opt  -codegenprepare -S -mcpu=kryo < %s  | FileCheck %s

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-linaro-linux-gnueabi"

; This test if a SelectInst in a loop latch is turned into an explicit branch
; in codegenprepare pass. This test case was an input IR of codegenprepare
; when compiling the C code below in -O3.
;
;struct s1 {int idx; int cost;};
;void foo(struct s1 **G, int n) {
; int j, i;
; while (j < n) {
;   if (G[j+1]->cost < G[j]->cost)
;     j++;
;   if (G[j]->cost > G[i]->cost)
;     break;
;  }
;}
;

%struct.s1 = type { i32, i32 }
;
; CHECK-LABEL: @foo
; CHECK-LABEL: while.body:
; CHECK-NOT: select i1
; CHECK: [[CMP:%.*]] = icmp slt i32 %l2, %l4
; CHECK: br i1 [[CMP]], label %select.end, label %select.false
;
define void @foo(i32 %n, %struct.s1** nocapture %G) local_unnamed_addr #0 {
entry:
  ;%0 = load i32, i32* @n, align 4
  ;%1 = load %struct.s1**, %struct.s1*** @G, align 8
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %j.0 = phi i32 [ undef, %entry ], [ %add.j.0, %while.body ]
  %cmp = icmp slt i32 %j.0, %n
  br i1 %cmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %add = add nsw i32 %j.0, 1
  %idxprom = sext i32 %add to i64
  %arrayidx = getelementptr inbounds %struct.s1*, %struct.s1** %G, i64 %idxprom
  %l = load %struct.s1*, %struct.s1** %arrayidx, align 8
  %cost = getelementptr inbounds %struct.s1, %struct.s1* %l, i64 0, i32 0
  %l2 = load i32, i32* %cost, align 4
  %idxprom1 = sext i32 %j.0 to i64
  %arrayidx2 = getelementptr inbounds %struct.s1*, %struct.s1** %G, i64 %idxprom1
  %l3 = load %struct.s1*, %struct.s1** %arrayidx2, align 8
  %cost3 = getelementptr inbounds %struct.s1, %struct.s1* %l3, i64 0, i32 0
  %l4 = load i32, i32* %cost3, align 4
  %cmp4 = icmp slt i32 %l2, %l4
  %add.j.0 = select i1 %cmp4, i32 %add, i32 %j.0
  %idxprom5 = sext i32 %add.j.0 to i64
  %arrayidx6 = getelementptr inbounds %struct.s1*, %struct.s1** %G, i64 %idxprom5
  %l5 = load %struct.s1*, %struct.s1** %arrayidx6, align 8
  %cost7 = getelementptr inbounds %struct.s1, %struct.s1* %l5, i64 0, i32 0
  %l6 = load i32, i32* %cost7, align 4
  %l7 = load %struct.s1*, %struct.s1** %G, align 8
  %cost10 = getelementptr inbounds %struct.s1, %struct.s1* %l7, i64 0, i32 0
  %l8 = load i32, i32* %cost10, align 4
  %cmp11 = icmp sgt i32 %l6, %l8
  br i1 %cmp11, label %while.end, label %while.cond

while.end:                                        ; preds = %while.body, %while.cond
  ret void
}

