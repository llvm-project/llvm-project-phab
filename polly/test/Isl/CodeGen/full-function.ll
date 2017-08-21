; RUN: opt %loadPolly -polly-codegen -polly-detect-full-functions -S \
; RUN:     < %s | FileCheck %s
; CHECK:      polly.split:
; CHECK:      polly.start:
; CHECK:      polly.stmt.bb2:

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
