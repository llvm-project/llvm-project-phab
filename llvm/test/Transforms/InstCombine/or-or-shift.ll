; RUN: opt -S -instcombine < %s | FileCheck %s

; (((A & 2) >> 1) | ((A & 2) << 6)) | (((A & 4) >> 1) | ((A & 4) << 6))
; --> (((A & 2) >> 1) | ((A & 4) >> 1)) | (((A & 2) << 6) | ((A & 4) << 6))
; --> (((A & 2) >> 1) | ((A & 4) >> 1)) | (((A & 2) | (A & 4)) << 6)
; --> (((A & 2) >> 1) | ((A & 4) >> 1)) | ((A & 6) << 6)
; --> (((A & 2) >> 1) | ((A & 4) >> 1)) | ((A << 6) & 384)
; --> (((A & 2) | (A & 4)) >> 1) | ((A << 6) & 384)
; --> ((A & 6) >> 1) | ((A << 6) & 384)
; --> ((A >> 1) & 3) | ((A << 6) & 384)
; --> ((A & 6) >> 1) | ((A & 6) << 6)

define i32 @or_or_shift(i32 %x) local_unnamed_addr #0  {
; CHECK-LABEL: @or_or_shift(
; CHECK-NEXT:    [[AND:%.*]] = and i32 %x, 6
; CHECK-NEXT:    [[SHL:%.*]] = shl nuw nsw i32 [[AND]], 6
; CHECK-NEXT:    [[SHR:%.*]] = lshr exact i32 [[AND]], 1
; CHECK-NEXT:    [[OR:%.*]] = or i32 [[SHL]], [[SHR]]
; CHECK-NEXT:    ret i32 [[OR]]
;
  %1 = and i32 %x, 2
  %2 = and i32 %x, 4
  %3 = shl nuw nsw i32 %1, 6
  %4 = lshr exact i32 %1, 1
  %5 = or i32 %3, %4
  %6 = shl nuw nsw i32 %2, 6
  %7 = lshr exact i32 %2, 1
  %8 = or i32 %6, %7
  %9 = or i32 %5, %8
  ret i32 %9
}

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

