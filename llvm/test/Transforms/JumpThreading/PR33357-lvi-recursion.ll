; RUN: opt -S -jump-threading -verify -o - %s | FileCheck %s

; Don't enter infinite recursion in LazyValueInfo.cpp

@a = external global i16, align 1

define void @f(i32 %p1) {
bb0:
  %0 = icmp eq i32 %p1, 0
  br i1 undef, label %bb6, label %bb1

bb1:                                         ; preds = %bb0
  br label %bb2

bb2:                                         ; preds = %bb4, %bb1
  %1 = phi i1 [ %0, %bb1 ], [ %2, %bb4 ]
  %2 = and i1 %1, undef
  br i1 %2, label %bb3, label %bb4

bb3:                                         ; preds = %bb2
  store i16 undef, i16* @a, align 1
  br label %bb4

bb4:                                         ; preds = %bb3, %bb2
  br i1 %0, label %bb2, label %bb5

bb5:                                         ; preds = %bb4
  unreachable

bb6:                                         ; preds = %bb0
  ret void
}

; The
;  br i1 undef, label %bb6, label %bb1
; is replaced by
;  br label %bb6
; and the rest of the function is unreachable from entry.

; CHECK:      define void @f(i32 %p1) {
; CHECK-NEXT: bb6:
; CHECK-NEXT:   ret void
