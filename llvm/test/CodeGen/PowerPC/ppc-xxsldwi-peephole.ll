; RUN: llc < %s -mtriple=powerpc64le-unknown-linux-gnu -mcpu=pwr9 -verify-machineinstrs | FileCheck %s
; RUN: llc < %s -mtriple=powerpc64le-unknown-linux-gnu -mcpu=pwr8 -verify-machineinstrs | FileCheck %s

; Function Attrs: norecurse nounwind readnone
define <16 x i8> @test(<16 x i8> %a, i8 zeroext %b) local_unnamed_addr {
entry:
  %0 = bitcast <16 x i8> %a to <4 x i32>
  %1 = shufflevector <4 x i32> %0, <4 x i32> undef, <4 x i32> <i32 3, i32 0, i32 1, i32 2>
  %2 = bitcast <4 x i32> %1 to <16 x i8>
  %add = add <16 x i8> %2, %a
  %splat.splat = shufflevector <16 x i8> %2, <16 x i8> undef, <16 x i32> <i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3>
  %add1 = add <16 x i8> %add, %splat.splat
  ret <16 x i8> %add1
; CHECK-LABEL: test
; CHECK: vspltb
; CHECK: blr
}

