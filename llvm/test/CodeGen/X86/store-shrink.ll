; RUN: opt < %s -mtriple=x86_64-unknown-linux-gnu -memaccessshrink -S | FileCheck %s
; Check bitfield store is shrinked properly in cases below.

; class A1 {
;   unsigned long f1:8;
;   unsigned long f2:3;
; } a1;
; a1.f1 = n;
;
; The bitfield store can be shrinked from i16 to i8.
; CHECK-LABEL: @test1(
; CHECK: %conv = zext i32 %n to i64
; CHECK: %t0 = trunc i64 %conv to i16
; CHECK: %bf.value = and i16 %t0, 255
; CHECK: %trunc = trunc i16 %bf.value to i8
; CHECK: store i8 %trunc, i8* bitcast (%class.A1* @a1 to i8*), align 8

%class.A1 = type { i16, [6 x i8] }
@a1 = local_unnamed_addr global %class.A1 zeroinitializer, align 8

define void @test1(i32 %n) {
entry:
  %conv = zext i32 %n to i64
  %t0 = trunc i64 %conv to i16
  %bf.load = load i16, i16* getelementptr inbounds (%class.A1, %class.A1* @a1, i32 0, i32 0), align 8
  %bf.value = and i16 %t0, 255
  %bf.clear = and i16 %bf.load, -256
  %bf.set = or i16 %bf.clear, %bf.value
  store i16 %bf.set, i16* getelementptr inbounds (%class.A1, %class.A1* @a1, i32 0, i32 0), align 8
  ret void
}

; class A2 {
;   unsigned long f1:16;
;   unsigned long f2:3;
; } a2;
; a2.f1 = n;
; The bitfield store can be shrinked from i32 to i16.
; CHECK-LABEL: @test2(
; CHECK: %bf.value = and i32 %n, 65535
; CHECK: %trunc = trunc i32 %bf.value to i16
; CHECK: store i16 %trunc, i16* bitcast (%class.A2* @a2 to i16*), align 8

%class.A2 = type { i24, [4 x i8] }
@a2 = local_unnamed_addr global %class.A2 zeroinitializer, align 8

define void @test2(i32 %n) {
entry:
  %bf.load = load i32, i32* bitcast (%class.A2* @a2 to i32*), align 8
  %bf.value = and i32 %n, 65535
  %bf.clear = and i32 %bf.load, -65536
  %bf.set = or i32 %bf.clear, %bf.value
  store i32 %bf.set, i32* bitcast (%class.A2* @a2 to i32*), align 8
  ret void
}

; class A3 {
;   unsigned long f1:32;
;   unsigned long f2:3;
; } a3;
; a3.f1 = n;
; The bitfield store can be shrinked from i64 to i32.
; CHECK-LABEL: @test3(
; CHECK: %conv = zext i32 %n to i64
; CHECK: %bf.value = and i64 %conv, 4294967295
; CHECK: %trunc = trunc i64 %bf.value to i32
; CHECK: store i32 %trunc, i32* bitcast (%class.A3* @a3 to i32*), align 8

%class.A3 = type { i40 }
@a3 = local_unnamed_addr global %class.A3 zeroinitializer, align 8

define void @test3(i32 %n) {
entry:
  %conv = zext i32 %n to i64
  %bf.load = load i64, i64* bitcast (%class.A3* @a3 to i64*), align 8
  %bf.value = and i64 %conv, 4294967295
  %bf.clear = and i64 %bf.load, -4294967296
  %bf.set = or i64 %bf.clear, %bf.value
  store i64 %bf.set, i64* bitcast (%class.A3* @a3 to i64*), align 8
  ret void
}

; class A4 {
;   unsigned long f1:13;
;   unsigned long f2:3;
; } a4;
; a4.f1 = n;
; The bitfield store cannot be shrinked because the field is not 8/16/32 bits.
; CHECK-LABEL: @test4(
; CHECK-NEXT: entry:
; CHECK-NEXT: %bf.load = load i16, i16* getelementptr inbounds (%class.A4, %class.A4* @a4, i64 0, i32 0), align 8
; CHECK-NEXT: %t0 = trunc i32 %n to i16
; CHECK-NEXT: %bf.value = and i16 %t0, 8191
; CHECK-NEXT: %bf.clear3 = and i16 %bf.load, -8192
; CHECK-NEXT: %bf.set = or i16 %bf.clear3, %bf.value
; CHECK-NEXT: store i16 %bf.set, i16* getelementptr inbounds (%class.A4, %class.A4* @a4, i64 0, i32 0), align 8
; CHECK-NEXT: ret void

%class.A4 = type { i16, [6 x i8] }
@a4 = local_unnamed_addr global %class.A4 zeroinitializer, align 8

define void @test4(i32 %n) {
entry:
  %bf.load = load i16, i16* getelementptr inbounds (%class.A4, %class.A4* @a4, i64 0, i32 0), align 8
  %t0 = trunc i32 %n to i16
  %bf.value = and i16 %t0, 8191
  %bf.clear3 = and i16 %bf.load, -8192
  %bf.set = or i16 %bf.clear3, %bf.value
  store i16 %bf.set, i16* getelementptr inbounds (%class.A4, %class.A4* @a4, i64 0, i32 0), align 8
  ret void
}

; class A5 {
;   unsigned long f1:3;
;   unsigned long f2:16;
; } a5;
; a5.f2 = n;
; The bitfield store cannot be shrinked because it is not aligned on
; 16bits boundary.
; CHECK-LABEL: @test5(
; CHECK-NEXT: entry:
; CHECK-NEXT: %bf.load = load i32, i32* bitcast (%class.A5* @a5 to i32*), align 8
; CHECK-NEXT: %bf.value = and i32 %n, 65535
; CHECK-NEXT: %bf.shl = shl i32 %bf.value, 3
; CHECK-NEXT: %bf.clear = and i32 %bf.load, -524281
; CHECK-NEXT: %bf.set = or i32 %bf.clear, %bf.shl
; CHECK-NEXT: store i32 %bf.set, i32* bitcast (%class.A5* @a5 to i32*), align 8
; CHECK-NEXT: ret void

%class.A5 = type { i24, [4 x i8] }
@a5 = local_unnamed_addr global %class.A5 zeroinitializer, align 8

define void @test5(i32 %n) {
entry:
  %bf.load = load i32, i32* bitcast (%class.A5* @a5 to i32*), align 8
  %bf.value = and i32 %n, 65535
  %bf.shl = shl i32 %bf.value, 3
  %bf.clear = and i32 %bf.load, -524281
  %bf.set = or i32 %bf.clear, %bf.shl
  store i32 %bf.set, i32* bitcast (%class.A5* @a5 to i32*), align 8
  ret void
}

; class A6 {
;   unsigned long f1:16;
;   unsigned long f2:3;
; } a6;
; a6.f1 = n;
; The bitfield store can be shrinked from i32 to i16 even the load and store
; are in different BasicBlocks.
; CHECK-LABEL: @test6(
; CHECK: if.end:
; CHECK: %bf.value = and i32 %n, 65535
; CHECK: %trunc = trunc i32 %bf.value to i16 
; CHECK: store i16 %trunc, i16* bitcast (%class.A6* @a6 to i16*), align 8

%class.A6 = type { i24, [4 x i8] }
@a6 = local_unnamed_addr global %class.A6 zeroinitializer, align 8

define void @test6(i32 %n) {
entry:
  %bf.load = load i32, i32* bitcast (%class.A6* @a6 to i32*), align 8
  %bf.clear = and i32 %bf.load, 65535
  %cmp = icmp eq i32 %bf.clear, 2
  br i1 %cmp, label %return, label %if.end

if.end:                                           ; preds = %entry
  %bf.value = and i32 %n, 65535
  %bf.clear3 = and i32 %bf.load, -65536
  %bf.set = or i32 %bf.clear3, %bf.value
  store i32 %bf.set, i32* bitcast (%class.A6* @a6 to i32*), align 8
  br label %return

return:                                           ; preds = %entry, %if.end
  ret void
}

; class A7 {
;   unsigned long f1:16;
;   unsigned long f2:16;
; } a7;
; a7.f2 = n;
; The bitfield store can be shrinked from i32 to i16.
; CHECK-LABEL: @test7(
; CHECK: %bf.value = and i32 %n, 65535
; CHECK: %bf.shl = shl i32 %bf.value, 16
; CHECK: %lshr = lshr i32 %bf.shl, 16
; CHECK: %trunc = trunc i32 %lshr to i16
; CHECK: store i16 %trunc, i16* bitcast (i8* getelementptr (i8, i8* bitcast (%class.A7* @a7 to i8*), i32 2) to i16*), align 2

%class.A7 = type { i32, [4 x i8] }
@a7 = local_unnamed_addr global %class.A7 zeroinitializer, align 8

define void @test7(i32 %n) {
entry:
  %bf.load = load i32, i32* getelementptr inbounds (%class.A7, %class.A7* @a7, i32 0, i32 0), align 8
  %bf.value = and i32 %n, 65535
  %bf.shl = shl i32 %bf.value, 16
  %bf.clear = and i32 %bf.load, 65535
  %bf.set = or i32 %bf.clear, %bf.shl
  store i32 %bf.set, i32* getelementptr inbounds (%class.A7, %class.A7* @a7, i32 0, i32 0), align 8
  ret void
}

; Cannot remove the load and bit operations, but can still shrink the i24 store
; to i16.
; CHECK-LABEL: @i24_or(
; CHECK: %cast = bitcast i24* %a to i16*
; CHECK: %load.trunc = load i16, i16* %cast, align 1
; CHECK: %or.trunc = or i16 %load.trunc, 384
; CHECK: store i16 %or.trunc, i16* %cast, align 1 
;
define void @i24_or(i24* %a) {
  %aa = load i24, i24* %a, align 1
  %b = or i24 %aa, 384
  store i24 %b, i24* %a, align 1
  ret void
}

; Cannot remove the load and bit operations, but can still shrink the i24 store
; to i8.
; CHECK-LABEL: @i24_and(
; CHECK: %cast = bitcast i24* %a to i8*
; CHECK: %uglygep = getelementptr i8, i8* %cast, i32 1
; CHECK: %load.trunc = load i8, i8* %uglygep, align 1
; CHECK: %and.trunc = and i8 %load.trunc, -7
; CHECK: store i8 %and.trunc, i8* %uglygep, align 1
;
define void @i24_and(i24* %a) {
  %aa = load i24, i24* %a, align 1
  %b = and i24 %aa, -1537
  store i24 %b, i24* %a, align 1
  ret void
}

; Cannot remove the load and bit operations, but can still shrink the i24 store
; to i16.
; CHECK-LABEL: @i24_xor(
; CHECK: %cast = bitcast i24* %a to i16*
; CHECK: %load.trunc = load i16, i16* %cast, align 1
; CHECK: %xor.trunc = xor i16 %load.trunc, 384
; CHECK: store i16 %xor.trunc, i16* %cast, align 1
;
define void @i24_xor(i24* %a) {
  %aa = load i24, i24* %a, align 1
  %b = xor i24 %aa, 384
  store i24 %b, i24* %a, align 1
  ret void
}

; Cannot remove the load and bit operations, but can still shrink the i24 store
; to i16.
; CHECK-LABEL: @i24_and_or(
; CHECK: %cast = bitcast i24* %a to i16*
; CHECK: %load.trunc = load i16, i16* %cast, align 1
; CHECK: %and.trunc = and i16 %load.trunc, -128
; CHECK: %or.trunc = or i16 %and.trunc, 384
; CHECK: store i16 %or.trunc, i16* %cast, align 1
;
define void @i24_and_or(i24* %a) {
  %b = load i24, i24* %a, align 1
  %c = and i24 %b, -128
  %d = or i24 %c, 384
  store i24 %d, i24* %a, align 1
  ret void
}

; Cannot remove the load and bit operations, but can shrink the i24 store to i8.
; CHECK-LABEL: @i24_insert_bit(
; CHECK: %extbit = zext i1 %bit to i24
; CHECK: %extbit.shl = shl nuw nsw i24 %extbit, 13
; CHECK: %cast = bitcast i24* %a to i8*
; CHECK: %uglygep = getelementptr i8, i8* %cast, i32 1
; CHECK: %load.trunc = load i8, i8* %uglygep, align 1
; CHECK: %lshr = lshr i24 %extbit.shl, 8
; CHECK: %trunc = trunc i24 %lshr to i8
; CHECK: %and.trunc = and i8 %load.trunc, -33
; CHECK: %or.trunc = or i8 %and.trunc, %trunc
; CHECK: store i8 %or.trunc, i8* %uglygep, align 1
;
define void @i24_insert_bit(i24* %a, i1 zeroext %bit) { 
  %extbit = zext i1 %bit to i24 
  %b = load i24, i24* %a, align 1 
  %extbit.shl = shl nuw nsw i24 %extbit, 13 
  %c = and i24 %b, -8193 
  %d = or i24 %c, %extbit.shl 
  store i24 %d, i24* %a, align 1 
  ret void 
}

; Cannot remove the load and bit operations, but can still shrink the i56 store
; to i16.
; CHECK-LABEL: @i56_or(
; CHECK: %cast = bitcast i56* %a to i16*
; CHECK: %load.trunc = load i16, i16* %cast, align 1
; CHECK: %or.trunc = or i16 %load.trunc, 384
; CHECK: store i16 %or.trunc, i16* %cast, align 1
;
define void @i56_or(i56* %a) {
  %aa = load i56, i56* %a, align 1
  %b = or i56 %aa, 384
  store i56 %b, i56* %a, align 1
  ret void
}

; Cannot remove the load and bit operations, but can shrink the i56 store
; to i16.
; CHECK-LABEL: @i56_and_or(
; CHECK: %cast = bitcast i56* %a to i16*
; CHECK: %load.trunc = load i16, i16* %cast, align 1
; CHECK: %and.trunc = and i16 %load.trunc, -128
; CHECK: %or.trunc = or i16 %and.trunc, 384
; CHECK: store i16 %or.trunc, i16* %cast, align 1
;
define void @i56_and_or(i56* %a) {
  %b = load i56, i56* %a, align 1
  %c = and i56 %b, -128
  %d = or i56 %c, 384
  store i56 %d, i56* %a, align 1
  ret void
}

; Cannot remove the load and bit operations, but can shrink the i56 store to i8.
; CHECK-LABEL: @i56_insert_bit(
; CHECK: %extbit = zext i1 %bit to i56
; CHECK: %extbit.shl = shl nuw nsw i56 %extbit, 13
; CHECK: %cast = bitcast i56* %a to i8*
; CHECK: %uglygep = getelementptr i8, i8* %cast, i32 1
; CHECK: %load.trunc = load i8, i8* %uglygep, align 1
; CHECK: %lshr = lshr i56 %extbit.shl, 8
; CHECK: %trunc = trunc i56 %lshr to i8
; CHECK: %and.trunc = and i8 %load.trunc, -33
; CHECK: %or.trunc = or i8 %and.trunc, %trunc
; CHECK: store i8 %or.trunc, i8* %uglygep, align 1
;
define void @i56_insert_bit(i56* %a, i1 zeroext %bit) {
  %extbit = zext i1 %bit to i56
  %b = load i56, i56* %a, align 1
  %extbit.shl = shl nuw nsw i56 %extbit, 13
  %c = and i56 %b, -8193
  %d = or i56 %c, %extbit.shl
  store i56 %d, i56* %a, align 1
  ret void
}

; Cannot remove the load and bit operations, but can still shrink the i56 store
; to i16.
; CHECK-LABEL: @i56_or_alg2(
; CHECK: %cast = bitcast i56* %a to i8*
; CHECK: %uglygep = getelementptr i8, i8* %cast, i32 2
; CHECK: %cast1 = bitcast i8* %uglygep to i16*
; CHECK: %load.trunc = load i16, i16* %cast1, align 2
; CHECK: %or.trunc = or i16 %load.trunc, 272
; CHECK: store i16 %or.trunc, i16* %cast1, align 2
;
define void @i56_or_alg2(i56* %a) {
  %aa = load i56, i56* %a, align 2
  %b = or i56 %aa, 17825792
  store i56 %b, i56* %a, align 2
  ret void
}


