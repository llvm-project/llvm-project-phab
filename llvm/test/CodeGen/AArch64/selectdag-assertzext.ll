; RUN: llc < %s -mtriple=aarch64--linux-gnu | FileCheck %s

; This test is to check an AND instruction for zext is not generated in %while.end
; by adding AssertZext to CopyFromReg for %idx while building DAG.

; CHECK-LABEL: @checkLiveoutInfo
; CHECK: ldrb w[[LD:[0-9]+]], [x1]
; CHECK-LABEL: %while.end
; CHECK-NOT: and w{{[0-9]+}}, w[[LD]], #0xff

@CA = common global i8* null, align 8

define i32 @checkLiveoutInfo(i64 %c,i8* %s) {
entry:
  %l1 = load i8, i8* %s, align 1
  %cmp12 = icmp eq i8 %l1, 0
  br i1 %cmp12, label %while.end, label %land.rhs.preheader

land.rhs.preheader:                               ; preds = %entry
  %scevgep = getelementptr i8, i8* %s, i64 -1
  br label %land.rhs

land.rhs:                                         ; preds = %land.rhs.preheader, %while.body
  %lsr.iv = phi i8* [ %scevgep, %land.rhs.preheader ], [ %scevgep16, %while.body ]
  %invalidatedPhi = phi i8 [ %l2, %while.body ], [ %l1, %land.rhs.preheader ]
  %cmp4 = icmp eq i64 %c, 0
  br i1 %cmp4, label %while.body, label %while.end

while.body:                                       ; preds = %land.rhs
  %l2 = load i8, i8* %lsr.iv, align 1
  %cmp = icmp eq i8 %l2, 0
  %scevgep16 = getelementptr i8, i8* %lsr.iv, i64 -1
  br i1 %cmp, label %while.end, label %land.rhs

while.end:                                        ; preds = %land.rhs, %while.body, %entry
  %idx= phi i8 [ 0, %entry ], [ %invalidatedPhi, %land.rhs ], [ 0, %while.body ]
  %l3 = load i8*, i8** @CA, align 8
  %zext1 = zext i8 %idx to i64
  %arrayidx = getelementptr inbounds i8, i8* %l3, i64 %zext1
  %l4 = load i8, i8* %arrayidx, align 1
  %zext2 = zext i8 %l4  to i32
  ret i32 %zext2
}



