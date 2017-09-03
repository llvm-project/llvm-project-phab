;RUN: opt < %s  -loop-vectorize  -S | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-grtev4-linux-gnu"

define void @foo() {
; CHECK-LABEL: @foo
; CHECK: vector.body:
bb:
  br i1 undef, label %bb1, label %bb9

bb1:                                              ; preds = %bb8, %bb
  br label %bb2

bb2:                                              ; preds = %bb2, %bb1
  %tmp = phi i16* [ undef, %bb1 ], [ %tmp6, %bb2 ]
  %tmp3 = phi i64 [ 0, %bb1 ], [ %tmp5, %bb2 ]
  %tmp4 = load i16, i16* %tmp, align 2
  %tmp5 = add nuw nsw i64 %tmp3, 1
  %tmp6 = getelementptr inbounds i16, i16* undef, i64 %tmp5
  %tmp7 = icmp eq i64 %tmp5, 65535
  br i1 %tmp7, label %bb8, label %bb2

bb8:                                              ; preds = %bb2
  br i1 undef, label %bb1, label %bb9

bb9:                                              ; preds = %bb8, %bb
  br i1 undef, label %bb10, label %bb12

bb10:                                             ; preds = %bb10, %bb9
  br i1 false, label %bb10, label %bb11

bb11:                                             ; preds = %bb10
  br label %bb12

bb12:                                             ; preds = %bb11, %bb9
  ret void
}

define void @foo2(i16 *%t1) { 
; CHECK-LABEL: @foo2
; CHECK: vector.body:
bb:
  br i1 undef, label %bb1, label %bb9

bb1:                                              ; preds = %bb8, %bb
  br label %bb2

bb2:                                              ; preds = %bb2, %bb1
  %tmp = phi i16* [ %t1, %bb1 ], [ %tmp6, %bb2 ]
  %tmp3 = phi i64 [ 0, %bb1 ], [ %tmp5, %bb2 ]
  %tmp4 = load i16, i16* %tmp, align 2
  %tmp5 = add nuw nsw i64 %tmp3, 1
  %tmp6 = getelementptr inbounds i16, i16* %t1, i64 %tmp5
  %tmp7 = icmp eq i64 %tmp5, 65535
  br i1 %tmp7, label %bb8, label %bb2

bb8:                                              ; preds = %bb2
  br i1 undef, label %bb1, label %bb9

bb9:                                              ; preds = %bb8, %bb
  br i1 undef, label %bb10, label %bb12

bb10:                                             ; preds = %bb10, %bb9
  br i1 false, label %bb10, label %bb11

bb11:                                             ; preds = %bb10
  br label %bb12

bb12:                                             ; preds = %bb11, %bb9
  ret void
}


