; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck %s
 
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Check that the loop is rotated.
; CHECK-LABEL: bar
; CHECK: bb4.lr:                                           ; preds = %bb
; CHECK: bb6.lr.ph:                                        ; preds = %bb4.lr
; CHECK: bb6:                                              ; preds = %bb6.lr.ph, %bb4
; CHECK: bb4:                                              ; preds = %bb6
; CHECK: bb12:                                             ; preds = %bb4.lr, %bb4

%0 = type { i64, i64, i8* }
%1 = type { i8 }

declare void @foo(%0*, %0*)

define linkonce_odr void @bar(%1* %arg, %0* %arg1, %0* %arg2, %0** %arg3) {
bb:
  br label %bb4

bb4:                                              ; preds = %bb6, %bb
  %tmp = phi %0* [ %arg2, %bb ], [ %tmp9, %bb6 ]
  %tmp5 = icmp eq %0* %tmp, %arg1
  br i1 %tmp5, label %bb12, label %bb6

bb6:                                              ; preds = %bb4
  %tmp7 = load %0*, %0** %arg3, align 8
  %tmp8 = getelementptr inbounds %0, %0* %tmp7, i64 -1
  %tmp9 = getelementptr inbounds %0, %0* %tmp, i64 -1
  tail call void @foo(%0* %tmp8, %0* nonnull dereferenceable(24) %tmp9)
  %tmp10 = load %0*, %0** %arg3, align 8
  %tmp11 = getelementptr inbounds %0, %0* %tmp10, i64 -1
  store %0* %tmp11, %0** %arg3, align 8
  br label %bb4

bb12:                                             ; preds = %bb4
  ret void
}
