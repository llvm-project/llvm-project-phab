; RUN: opt < %s -instcombine -S | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

; class A1 {
;   unsigned long f1:8;
;   unsigned long f2:3;
; } a1;
; a1.f1 = n;
;
; The bitfield store can be shrinked from i16 to i8.
; CHECK-LABEL: @test1(
; CHECK-NEXT: entry:
; CHECK-NEXT: %[[TRUNC:.*]] = trunc i32 %n to i8
; CHECK-NEXT: store i8 %[[TRUNC]], i8* bitcast (%class.A1* @a1 to i8*)
; CHECK-NEXT: ret void

%class.A1 = type { i16, [6 x i8] }
@a1 = local_unnamed_addr global %class.A1 zeroinitializer, align 8

define void @test1(i32 %n) {
entry:
  %conv = zext i32 %n to i64
  %0 = trunc i64 %conv to i16
  %bf.load = load i16, i16* getelementptr inbounds (%class.A1, %class.A1* @a1, i32 0, i32 0), align 8
  %bf.value = and i16 %0, 255
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
; CHECK-NEXT: entry:
; CHECK-NEXT: %[[TRUNC:.*]] = trunc i32 %n to i16
; CHECK-NEXT: store i16 %[[TRUNC]], i16* bitcast (%class.A2* @a2 to i16*)
; CHECK-NEXT: ret void

%class.A2 = type { i24, [4 x i8] }
@a2 = local_unnamed_addr global %class.A2 zeroinitializer, align 8

define void @test2(i32 %n) {
entry:
  %conv = zext i32 %n to i64
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
; CHECK-NEXT: entry:
; CHECK-NEXT: store i32 %n, i32* bitcast (%class.A3* @a3 to i32*)
; CHECK-NEXT: ret void

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
; CHECK-NEXT: %bf.load = load i16, i16* getelementptr inbounds (%class.A4, %class.A4* @a4, i64 0, i32 0)
; CHECK-NEXT: %[[TRUNC:.*]] = trunc i32 %n to i16
; CHECK-NEXT: %bf.value = and i16 %[[TRUNC]], 8191
; CHECK-NEXT: %bf.clear3 = and i16 %bf.load, -8192
; CHECK-NEXT: %bf.set = or i16 %bf.clear3, %bf.value
; CHECK-NEXT: store i16 %bf.set, i16* getelementptr inbounds (%class.A4, %class.A4* @a4, i64 0, i32 0)
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
; CHECK-NEXT: %bf.load = load i32, i32* bitcast (%class.A5* @a5 to i32*)
; CHECK-NEXT: %bf.value = shl i32 %n, 3
; CHECK-NEXT: and i32 %bf.value, 524280
; CHECK-NEXT: %bf.clear = and i32 %bf.load, -524281
; CHECK-NEXT: %bf.set = or i32 %bf.clear, %bf.shl
; CHECK-NEXT: store i32 %bf.set, i32* bitcast (%class.A5* @a5 to i32*)
; CHECK-NEXT: ret void

%class.A5 = type { i24, [4 x i8] }
@a5 = local_unnamed_addr global %class.A5 zeroinitializer, align 8

define void @test5(i32 %n) {
entry:
  %conv = zext i32 %n to i64
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
; CHECK-NEXT: entry:
; CHECK-NEXT: %bf.load = load i32, i32* bitcast (%class.A6* @a6 to i32*)
; CHECK-NEXT: %bf.clear = and i32 %bf.load, 65535
; CHECK-NEXT: %cmp = icmp eq i32 %bf.clear, 2
; CHECK-NEXT: br i1 %cmp, label %return, label %if.end
; CHECK: if.end:
; CHECK-NEXT: %0 = trunc i32 %n to i16
; CHECK-NEXT: store i16 %0, i16* bitcast (%class.A6* @a6 to i16*)
; CHECK-NEXT: br label %return

%class.A6 = type { i24, [4 x i8] }
@a6 = local_unnamed_addr global %class.A6 zeroinitializer, align 8

define void @test6(i32 %n) {
entry:
  %bf.load = load i32, i32* bitcast (%class.A6* @a6 to i32*), align 8
  %bf.clear = and i32 %bf.load, 65535
  %bf.cast = zext i32 %bf.clear to i64
  %cmp = icmp eq i32 %bf.clear, 2
  br i1 %cmp, label %return, label %if.end

if.end:                                           ; preds = %entry
  %conv1 = zext i32 %n to i64
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
; CHECK-NEXT: entry:
; CHECK-NEXT: %[[TRUNC:.*]] = trunc i32 %n to i16
; CHECK-NEXT: store i16 %[[TRUNC]], i16* bitcast (i8* getelementptr (i8, i8* bitcast (%class.A7* @a7 to i8*), i64 2) to i16*)
; CHECK-NEXT: ret void

%class.A7 = type { i32, [4 x i8] }
@a7 = local_unnamed_addr global %class.A7 zeroinitializer, align 8

define void @test7(i32 %n) {
entry:
  %conv = zext i32 %n to i64
  %bf.load = load i32, i32* getelementptr inbounds (%class.A7, %class.A7* @a7, i32 0, i32 0), align 8
  %bf.value = and i32 %n, 65535
  %bf.shl = shl i32 %bf.value, 16
  %bf.clear = and i32 %bf.load, 65535
  %bf.set = or i32 %bf.clear, %bf.shl
  store i32 %bf.set, i32* getelementptr inbounds (%class.A7, %class.A7* @a7, i32 0, i32 0), align 8
  ret void
}

