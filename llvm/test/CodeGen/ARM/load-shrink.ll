; RUN: opt < %s -mtriple=x86_64-unknown-linux-gnu -memaccessshrink -S | FileCheck %s
; Check bitfield load is shrinked properly in cases below.

; The bitfield store can be shrinked from i64 to i16.
; CHECK-LABEL: @load_and(
; CHECK: %cast = bitcast i64* %ptr to i16*
; CHECK: %load.trunc = load i16, i16* %cast, align 8
; CHECK: %trunc.zext = zext i16 %load.trunc to i64
; CHECK: %cmp = icmp eq i64 %trunc.zext, 1

define i1 @load_and(i64* %ptr) {
entry:
  %bf.load = load i64, i64* %ptr, align 8
  %bf.clear = and i64 %bf.load, 65535
  %cmp = icmp eq i64 %bf.clear, 1
  ret i1 %cmp
}

; The bitfield store can be shrinked from i64 to i8.
; CHECK-LABEL: @load_trunc(
; CHECK: %cast = bitcast i64* %ptr to i8*
; CHECK: %load.trunc = load i8, i8* %cast, align 8
; CHECK: %cmp = icmp eq i8 %load.trunc, 1

define i1 @load_trunc(i64* %ptr) {
entry:
  %bf.load = load i64, i64* %ptr, align 8
  %bf.clear = trunc i64 %bf.load to i8
  %cmp = icmp eq i8 %bf.clear, 1
  ret i1 %cmp
}

; The bitfield store can be shrinked from i64 to i8.
; CHECK-LABEL: @load_and_shr(
; CHECK: %cast = bitcast i64* %ptr to i8*
; CHECK: %uglygep = getelementptr i8, i8* %cast, i32 6
; CHECK: %load.trunc = load i8, i8* %uglygep, align 2
; CHECK: %trunc.zext = zext i8 %load.trunc to i64
; CHECK: %cmp = icmp eq i64 %trunc.zext, 1

define i1 @load_and_shr(i64* %ptr) {
entry:
  %bf.load = load i64, i64* %ptr, align 8
  %bf.lshr = lshr i64 %bf.load, 48
  %bf.clear = and i64 %bf.lshr, 255
  %cmp = icmp eq i64 %bf.clear, 1
  ret i1 %cmp
}

; The bitfield store can be shrinked from i64 to i8.
; CHECK-LABEL: @load_and_shr_add(
; CHECK: %cast = bitcast i64* %ptr to i8*
; CHECK: %uglygep = getelementptr i8, i8* %cast, i32 6
; CHECK: %load.trunc = load i8, i8* %uglygep, align 2
; CHECK: %[[ADD:.*]] = add i8 %load.trunc, 1
; CHECK: %trunc.zext = zext i8 %[[ADD]] to i64
; CHECK: %cmp = icmp eq i64 %trunc.zext, 1

define i1 @load_and_shr_add(i64* %ptr) {
entry:
  %bf.load = load i64, i64* %ptr, align 8
  %add = add i64 %bf.load, 281474976710656
  %bf.lshr = lshr i64 %add, 48
  %bf.clear = and i64 %bf.lshr, 255
  %cmp = icmp eq i64 %bf.clear, 1
  ret i1 %cmp
}

; The bitfield store can be shrinked from i64 to i8.
; CHECK-LABEL: @load_ops(
; CHECK: %cast = bitcast i64* %ptr to i8*
; CHECK: %uglygep = getelementptr i8, i8* %cast, i32 6
; CHECK: %load.trunc = load i8, i8* %uglygep, align 2
; CHECK: %[[ADD:.*]] = add i8 %load.trunc, 1
; CHECK: %trunc.zext = zext i8 %[[ADD]] to i64
; CHECK: %cmp = icmp eq i64 %trunc.zext, 1

define i1 @load_ops(i64* %ptr, i64 %value) {
entry:
  %bf.load = load i64, i64* %ptr, align 8
  %bf.value = and i64 %value, 65535
  %bf.clear1 = and i64 %bf.load, -65536
  %bf.set = or i64 %bf.value, %bf.clear1
  %add = add i64 %bf.set, 281474976710656
  %bf.lshr = lshr i64 %add, 48
  %bf.clear = and i64 %bf.lshr, 255
  %cmp = icmp eq i64 %bf.clear, 1
  ret i1 %cmp
}

; It doesn't worth to do the shrink because %bf.load has multiple uses
; and the shrink here doesn't save instructions.
; CHECK-LABEL: @load_trunc_multiuses
; CHECK: %bf.load = load i64, i64* %ptr, align 8
; CHECK: %bf.trunc = trunc i64 %bf.load to i16
; CHECK: %cmp1 = icmp eq i16 %bf.trunc, 3
; CHECK: %cmp2 = icmp ult i64 %bf.load, 1500000
; CHECK: %cmp = and i1 %cmp1, %cmp2

define i1 @load_trunc_multiuses(i64* %ptr) {
entry:
  %bf.load = load i64, i64* %ptr, align 8
  %bf.trunc = trunc i64 %bf.load to i16
  %cmp1 = icmp eq i16 %bf.trunc, 3
  %cmp2 = icmp ult i64 %bf.load, 1500000
  %cmp = and i1 %cmp1, %cmp2
  ret i1 %cmp
}

