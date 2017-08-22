; RUN: opt %loadPolly -polly-codegen -polly-detect-full-functions -S \
; RUN:     < %s | FileCheck %s
; CHECK: bb:
; CHECK:  br i1 %3, label %bb.split, label %polly.split
; CHECK:  bb.split:
; CHECK:    br label %bb1
; CHECK:  b1:
; CHECK:    br i1 %exitcond, label %bb2, label %bb4polly.merge
; CHECK:  bb2:
; CHECK:    br label %bb3
; CHECK:  bb3:
; CHECK:    br label %bb1
; CHECK:  bb4polly.merge:
; CHECK:    br label %bb4
; CHECK:  bb4:
; CHECK:    ret void
; CHECK:  polly.split:
; CHECK:    br label %polly.start
; CHECK:  polly.start:
; CHECK:    br label %polly.loop_if
; CHECK:  polly.loop_exit:
; CHECK:    br label %polly.exiting
; CHECK:  polly.exiting:
; CHECK:    br label %bb4polly.merge
; CHECK:  polly.loop_if:
; CHECK:    br i1 %polly.loop_guard, label %polly.loop_preheader, label %polly.loop_exit
; CHECK:  polly.loop_header:
; CHECK:    br label %polly.stmt.bb2
; CHECK:  polly.stmt.bb2:
; CHECK:  br i1 %polly.loop_cond, label %polly.loop_header, label %polly.loop_exit
; CHECK:  polly.loop_preheader:
; CHECK:    br label %polly.loop_header

@A = common global [1024 x i32] zeroinitializer, align 16 ; <[1024 x i32]*> [#uses=3]

define void @bar(i64 %n) nounwind {
bb:
  br label %bb1

bb1:                                              ; preds = %bb3, %bb
  %i.0 = phi i64 [ 0, %bb ], [ %tmp, %bb3 ]       ; <i64> [#uses=3]
  %scevgep = getelementptr [1024 x i32], [1024 x i32]* @A, i64 0, i64 %i.0 ; <i32*> [#uses=1]
  %exitcond = icmp ne i64 %i.0, %n                ; <i1> [#uses=1]
  br i1 %exitcond, label %bb2, label %bb4

bb2:                                              ; preds = %bb1
  store i32 1, i32* %scevgep
  br label %bb3

bb3:                                              ; preds = %bb2
  %tmp = add nsw i64 %i.0, 1                      ; <i64> [#uses=1]
  br label %bb1

bb4:                                              ; preds = %bb1
  ret void
}
