; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -ppc-strip-register-prefix < %s | FileCheck %s
target triple = "powerpc64le-linux-gnu"

define i8 @test000(i8 %a, i8 %b) {
; CHECK-LABEL: test000:
; CHECK:       # BB#0:
; CHECK-NEXT:    rlwinm 4, 4, 0, 29, 31
; CHECK-NEXT:    slw 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i8 %b, 7
  %shl = shl i8 %a, %rem
  ret i8 %shl
}

define i16 @test001(i16 %a, i16 %b) {
; CHECK-LABEL: test001:
; CHECK:       # BB#0:
; CHECK-NEXT:    rlwinm 4, 4, 0, 28, 31
; CHECK-NEXT:    slw 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i16 %b, 15
  %shl = shl i16 %a, %rem
  ret i16 %shl
}

define i32 @test002(i32 %a, i32 %b) {
; CHECK-LABEL: test002:
; CHECK:       # BB#0:
; CHECK-NEXT:    rlwinm 4, 4, 0, 27, 31
; CHECK-NEXT:    slw 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i32 %b, 31
  %shl = shl i32 %a, %rem
  ret i32 %shl
}

define i64 @test003(i64 %a, i64 %b) {
; CHECK-LABEL: test003:
; CHECK:       # BB#0:
; CHECK-NEXT:    rlwinm 4, 4, 0, 26, 31
; CHECK-NEXT:    sld 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i64 %b, 63
  %shl = shl i64 %a, %rem
  ret i64 %shl
}

define <16 x i8> @test010(<16 x i8> %a, <16 x i8> %b) {
; CHECK-LABEL: test010:
; CHECK:       # BB#0:
; CHECK-NEXT:    vslb 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <16 x i8> %b, <i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7>
  %shl = shl <16 x i8> %a, %rem
  ret <16 x i8> %shl
}

define <8 x i16> @test011(<8 x i16> %a, <8 x i16> %b) {
; CHECK-LABEL: test011:
; CHECK:       # BB#0:
; CHECK-NEXT:    vslh 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <8 x i16> %b, <i16 15, i16 15, i16 15, i16 15, i16 15, i16 15, i16 15, i16 15>
  %shl = shl <8 x i16> %a, %rem
  ret <8 x i16> %shl
}

define <4 x i32> @test012(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: test012:
; CHECK:       # BB#0:
; CHECK-NEXT:    vslw 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <4 x i32> %b, <i32 31, i32 31, i32 31, i32 31>
  %shl = shl <4 x i32> %a, %rem
  ret <4 x i32> %shl
}

define <2 x i64> @test013(<2 x i64> %a, <2 x i64> %b) {
; CHECK-LABEL: test013:
; CHECK:       # BB#0:
; CHECK-NEXT:    vsld 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <2 x i64> %b, <i64 63, i64 63>
  %shl = shl <2 x i64> %a, %rem
  ret <2 x i64> %shl
}

define i8 @test100(i8 %a, i8 %b) {
; CHECK-LABEL: test100:
; CHECK:       # BB#0:
; CHECK-NEXT:    rlwinm 3, 3, 0, 24, 31
; CHECK-NEXT:    rlwinm 4, 4, 0, 29, 31
; CHECK-NEXT:    srw 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i8 %b, 7
  %lshr = lshr i8 %a, %rem
  ret i8 %lshr
}

define i16 @test101(i16 %a, i16 %b) {
; CHECK-LABEL: test101:
; CHECK:       # BB#0:
; CHECK-NEXT:    rlwinm 3, 3, 0, 16, 31
; CHECK-NEXT:    rlwinm 4, 4, 0, 28, 31
; CHECK-NEXT:    srw 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i16 %b, 15
  %lshr = lshr i16 %a, %rem
  ret i16 %lshr
}

define i32 @test102(i32 %a, i32 %b) {
; CHECK-LABEL: test102:
; CHECK:       # BB#0:
; CHECK-NEXT:    rlwinm 4, 4, 0, 27, 31
; CHECK-NEXT:    srw 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i32 %b, 31
  %lshr = lshr i32 %a, %rem
  ret i32 %lshr
}

define i64 @test103(i64 %a, i64 %b) {
; CHECK-LABEL: test103:
; CHECK:       # BB#0:
; CHECK-NEXT:    rlwinm 4, 4, 0, 26, 31
; CHECK-NEXT:    srd 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i64 %b, 63
  %lshr = lshr i64 %a, %rem
  ret i64 %lshr
}

define <16 x i8> @test110(<16 x i8> %a, <16 x i8> %b) {
; CHECK-LABEL: test110:
; CHECK:       # BB#0:
; CHECK-NEXT:    vsrb 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <16 x i8> %b, <i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7>
  %lshr = lshr <16 x i8> %a, %rem
  ret <16 x i8> %lshr
}

define <8 x i16> @test111(<8 x i16> %a, <8 x i16> %b) {
; CHECK-LABEL: test111:
; CHECK:       # BB#0:
; CHECK-NEXT:    vsrh 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <8 x i16> %b, <i16 15, i16 15, i16 15, i16 15, i16 15, i16 15, i16 15, i16 15>
  %lshr = lshr <8 x i16> %a, %rem
  ret <8 x i16> %lshr
}

define <4 x i32> @test112(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: test112:
; CHECK:       # BB#0:
; CHECK-NEXT:    vsrw 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <4 x i32> %b, <i32 31, i32 31, i32 31, i32 31>
  %lshr = lshr <4 x i32> %a, %rem
  ret <4 x i32> %lshr
}

define <2 x i64> @test113(<2 x i64> %a, <2 x i64> %b) {
; CHECK-LABEL: test113:
; CHECK:       # BB#0:
; CHECK-NEXT:    vsrd 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <2 x i64> %b, <i64 63, i64 63>
  %lshr = lshr <2 x i64> %a, %rem
  ret <2 x i64> %lshr
}

define i8 @test200(i8 %a, i8 %b) {
; CHECK-LABEL: test200:
; CHECK:       # BB#0:
; CHECK-NEXT:    extsb 3, 3
; CHECK-NEXT:    rlwinm 4, 4, 0, 29, 31
; CHECK-NEXT:    sraw 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i8 %b, 7
  %ashr = ashr i8 %a, %rem
  ret i8 %ashr
}

define i16 @test201(i16 %a, i16 %b) {
; CHECK-LABEL: test201:
; CHECK:       # BB#0:
; CHECK-NEXT:    extsh 3, 3
; CHECK-NEXT:    rlwinm 4, 4, 0, 28, 31
; CHECK-NEXT:    sraw 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i16 %b, 15
  %ashr = ashr i16 %a, %rem
  ret i16 %ashr
}

define i32 @test202(i32 %a, i32 %b) {
; CHECK-LABEL: test202:
; CHECK:       # BB#0:
; CHECK-NEXT:    rlwinm 4, 4, 0, 27, 31
; CHECK-NEXT:    sraw 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i32 %b, 31
  %ashr = ashr i32 %a, %rem
  ret i32 %ashr
}

define i64 @test203(i64 %a, i64 %b) {
; CHECK-LABEL: test203:
; CHECK:       # BB#0:
; CHECK-NEXT:    rlwinm 4, 4, 0, 26, 31
; CHECK-NEXT:    srad 3, 3, 4
; CHECK-NEXT:    blr
  %rem = and i64 %b, 63
  %ashr = ashr i64 %a, %rem
  ret i64 %ashr
}

define <16 x i8> @test210(<16 x i8> %a, <16 x i8> %b) {
; CHECK-LABEL: test210:
; CHECK:       # BB#0:
; CHECK-NEXT:    vsrab 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <16 x i8> %b, <i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7>
  %ashr = ashr <16 x i8> %a, %rem
  ret <16 x i8> %ashr
}

define <8 x i16> @test211(<8 x i16> %a, <8 x i16> %b) {
; CHECK-LABEL: test211:
; CHECK:       # BB#0:
; CHECK-NEXT:    vsrah 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <8 x i16> %b, <i16 15, i16 15, i16 15, i16 15, i16 15, i16 15, i16 15, i16 15>
  %ashr = ashr <8 x i16> %a, %rem
  ret <8 x i16> %ashr
}

define <4 x i32> @test212(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: test212:
; CHECK:       # BB#0:
; CHECK-NEXT:    vsraw 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <4 x i32> %b, <i32 31, i32 31, i32 31, i32 31>
  %ashr = ashr <4 x i32> %a, %rem
  ret <4 x i32> %ashr
}

define <2 x i64> @test213(<2 x i64> %a, <2 x i64> %b) {
; CHECK-LABEL: test213:
; CHECK:       # BB#0:
; CHECK-NEXT:    vsrad 2, 2, 3
; CHECK-NEXT:    blr
  %rem = and <2 x i64> %b, <i64 63, i64 63>
  %ashr = ashr <2 x i64> %a, %rem
  ret <2 x i64> %ashr
}
