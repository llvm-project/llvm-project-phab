; RUN: llc -mtriple=arm-eabi -float-abi=soft -mattr=+neon %s -o - | FileCheck %s

%M = type { [4 x <4 x float>] }

; Function Attrs: noimplicitfloat noinline norecurse nounwind uwtable
define void @_test_vld1_vst1(%M* %A, %M *%B)  {
entry:
  %v0p = getelementptr inbounds %M, %M* %A, i32 0, i32 0, i32 0
  %v0 = load <4 x float>, <4 x float>* %v0p
  %v1p = getelementptr inbounds %M, %M* %A, i32 0, i32 0, i32 1
  %v1 = load <4 x float>, <4 x float>* %v1p
  %v2p = getelementptr inbounds %M, %M* %A, i32 0, i32 0, i32 2
  %v2 = load <4 x float>, <4 x float>* %v2p
  %v3p = getelementptr inbounds %M, %M* %A, i32 0, i32 0, i32 3
  %v3 = load <4 x float>, <4 x float>* %v3p

  %s0p = getelementptr inbounds %M, %M* %B, i32 0, i32 0, i32 0
  store <4 x float> %v0, <4 x float>* %s0p
  %s1p = getelementptr inbounds %M, %M* %B, i32 0, i32 0, i32 1
  store <4 x float> %v1, <4 x float>* %s1p
  %s2p = getelementptr inbounds %M, %M* %B, i32 0, i32 0, i32 2
  store <4 x float> %v2, <4 x float>* %s2p
  %s3p = getelementptr inbounds %M, %M* %B, i32 0, i32 0, i32 3
  store <4 x float> %v3, <4 x float>* %s3p
  ret void
}

; CHECK:       vld1.32 {d16, d17}, [r0]!
; CHECK-NEXT:  vld1.32 {d18, d19}, [r0]!
; CHECK-NEXT:  vld1.32 {d20, d21}, [r0]!
; CHECK-NEXT:  vld1.64 {d22, d23}, [r0]
; CHECK-NEXT:  vst1.32 {d16, d17}, [r1]!
; CHECK-NEXT:  vst1.32 {d18, d19}, [r1]!
; CHECK-NEXT:  vst1.32 {d20, d21}, [r1]!
; CHECK-NEXT:  vst1.64 {d22, d23}, [r1]
; CHECK-NEXT:  mov pc, lr

