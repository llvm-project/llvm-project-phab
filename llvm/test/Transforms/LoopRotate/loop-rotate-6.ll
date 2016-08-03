; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck %s
 target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
 target triple = "x86_64-unknown-linux-gnu"

; Check that the loop is rotated.
; CHECK-LABEL: bar
; CHECK: bb19.preheader:                                   ; preds = %bb10, %bb10
; CHECK: bb19.lr:                                          ; preds = %bb19.preheader
; CHECK: bb12.lr.ph:                                       ; preds = %bb19.lr

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare i32 @foo(i8*, i32)

define i32 @bar(i8* %arg, i32 %arg1, i8* %arg2, i32 %arg3) {
bb:
  switch i32 %arg3, label %bb5 [
    i32 0, label %bb23
    i32 -1, label %bb4
  ]

bb4:                                              ; preds = %bb
  unreachable

bb5:                                              ; preds = %bb
  br i1 undef, label %bb6, label %bb7

bb6:                                              ; preds = %bb5
  br i1 undef, label %bb8, label %bb9

bb7:                                              ; preds = %bb5
  unreachable

bb8:                                              ; preds = %bb6
  unreachable

bb9:                                              ; preds = %bb6
  br i1 undef, label %bb22, label %bb10

bb10:                                             ; preds = %bb9
  switch i32 undef, label %bb11 [
    i32 46, label %bb19
    i32 32, label %bb19
  ]

bb11:                                             ; preds = %bb10
  unreachable

bb12:                                             ; preds = %bb19
  switch i8 undef, label %bb13 [
    i8 32, label %bb21
    i8 46, label %bb21
  ]

bb13:                                             ; preds = %bb12
  br i1 undef, label %bb14, label %bb15

bb14:                                             ; preds = %bb13
  unreachable

bb15:                                             ; preds = %bb13
  br i1 undef, label %bb17, label %bb16

bb16:                                             ; preds = %bb15
  %tmp = call i32 @foo(i8* %tmp20, i32 10)
  unreachable

bb17:                                             ; preds = %bb15
  %tmp18 = phi i8* [ %tmp20, %bb15 ]
  br label %bb19

bb19:                                             ; preds = %bb17, %bb10, %bb10
  %tmp20 = phi i8* [ %tmp18, %bb17 ], [ null, %bb10 ], [ null, %bb10 ]
  br i1 undef, label %bb21, label %bb12

bb21:                                             ; preds = %bb19, %bb12, %bb12
  unreachable

bb22:                                             ; preds = %bb9
  unreachable

bb23:                                             ; preds = %bb
  ret i32 0
}
