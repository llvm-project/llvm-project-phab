; RUN: llc -verify-machineinstrs -mtriple=powerpc64le-unknown-linux-gnu -mcpu=pwr8 < %s | FileCheck %s
; RUN: llc -verify-machineinstrs -mtriple=powerpc64-unknown-linux-gnu -mcpu=pwr8 < %s | FileCheck %s

; bitfieldinsert64: Test for rldimi
; equivalent C code
;   struct s64 {
;   	int a:5;
;   	int b:16;
;   	long c:42;
;   };
;   void bitfieldinsert64(struct s *p, unsigned short v) {
;   	p->b = v;
;   }

%struct.s64 = type { i64 }

define void @bitfieldinsert64(%struct.s64* nocapture %p, i16 zeroext %v) {
; CHECK-LABEL: @bitfieldinsert64
; CHECK: ld [[REG1:[0-9]+]], 0(3)
; CHECK: rldimi [[REG1]], 4, 5, 43
; CHECK: std [[REG1]], 0(3)
; CHECK: blr
entry:
  %0 = getelementptr inbounds %struct.s64, %struct.s64* %p, i64 0, i32 0
  %1 = zext i16 %v to i64
  %bf.load = load i64, i64* %0, align 8
  %bf.shl = shl nuw nsw i64 %1, 5
  %bf.clear = and i64 %bf.load, -2097121
  %bf.set = or i64 %bf.clear, %bf.shl
  store i64 %bf.set, i64* %0, align 8
  ret void
}

; bitfieldinsert32: Test for rlwimi
; equivalent C code
;   struct s32 {
;   	int a:8;
;   	int b:16;
;   	int c:8;
;   };
;   void bitfieldinsert32(struct s32 *p, unsigned int v) {
;   	p->b = v;
;   }

%struct.s32 = type { i32 }

define void @bitfieldinsert32(%struct.s32* nocapture %p, i32 zeroext %v) {
; CHECK-LABEL: @bitfieldinsert32
; CHECK: lwz [[REG1:[0-9]+]], 0(3)
; CHECK: rlwimi [[REG1]], 4, 8, 8, 23
; CHECK: stw [[REG1]], 0(3)
; CHECK: blr
entry:
  %0 = getelementptr inbounds %struct.s32, %struct.s32* %p, i64 0, i32 0
  %bf.load = load i32, i32* %0, align 4
  %bf.value = shl i32 %v, 8
  %bf.shl = and i32 %bf.value, 16776960
  %bf.clear = and i32 %bf.load, -16776961
  %bf.set = or i32 %bf.clear, %bf.shl
  store i32 %bf.set, i32* %0, align 4
  ret void
}

