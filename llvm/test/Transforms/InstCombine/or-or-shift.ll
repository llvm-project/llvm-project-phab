; RUN: opt -S -instcombine < %s | FileCheck %s

define i32 @or_and_shifts1(i32 %x) local_unnamed_addr #0  {
; CHECK-LABEL: @or_and_shifts1(
; CHECK-NEXT:    [[AND:%.*]] = and i32 %x, 1
; CHECK-NEXT:    [[SHL1:%.*]] = shl nuw nsw i32 [[AND]], 3
; CHECK-NEXT:    [[SHL2:%.*]] = shl nuw nsw i32 [[AND]], 5
; CHECK-NEXT:    [[OR:%.*]] = or i32 [[SHL1]], [[SHL2]]
; CHECK-NEXT:    ret i32 [[OR]]
;
  %1 = shl i32 %x, 3
  %2 = and i32 %1, 15
  %3 = shl i32 %x, 5
  %4 = and i32 %3, 60
  %5 = or i32 %2, %4
  ret i32 %5
}

define i32 @or_and_shifts2(i32 %x) local_unnamed_addr #0  {
; CHECK-LABEL: @or_and_shifts2(
; CHECK-NEXT:    [[AND:%.*]] = and i32 %x, 112
; CHECK-NEXT:    [[SHL:%.*]] = shl nuw nsw i32 [[AND]], 3
; CHECK-NEXT:    [[SHR:%.*]] = lshr exact i32 [[AND]], 4
; CHECK-NEXT:    [[OR:%.*]] = or i32 [[SHL]], [[SHR]]
; CHECK-NEXT:    ret i32 [[OR]]
;
  %1 = shl i32 %x, 3
  %2 = and i32 %1, 896
  %3 = lshr i32 %x, 4
  %4 = and i32 %3, 7
  %5 = or i32 %2, %4
  ret i32 %5
}

define i32 @or_and_shift_shift_and(i32 %x) local_unnamed_addr #0  {
; CHECK-LABEL: @or_and_shift_shift_and(
; CHECK-NEXT:    [[AND:%.*]] = and i32 %x, 7
; CHECK-NEXT:    [[SHL1:%.*]] = shl nuw nsw i32 [[AND]], 3
; CHECK-NEXT:    [[SHL2:%.*]] = shl nuw nsw i32 [[AND]], 2
; CHECK-NEXT:    [[OR:%.*]] = or i32 [[SHL1]], [[SHL2]]
; CHECK-NEXT:    ret i32 [[OR]]
;
  %1 = and i32 %x, 7
  %2 = shl i32 %1, 3
  %3 = shl i32 %x, 2
  %4 = and i32 %3, 28
  %5 = or i32 %2, %4
  ret i32 %5
}

