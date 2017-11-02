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

; CHECK:       vld1.32 {d{{[0-9]+}}, d{{[0-9]+}}}, [r0]!
; CHECK-NEXT:  vld1.32 {d{{[0-9]+}}, d{{[0-9]+}}}, [r0]!
; CHECK-NEXT:  vld1.32 {d{{[0-9]+}}, d{{[0-9]+}}}, [r0]!
; CHECK-NEXT:  vld1.64 {d{{[0-9]+}}, d{{[0-9]+}}}, [r0]
; CHECK-NEXT:  vst1.32 {d{{[0-9]+}}, d{{[0-9]+}}}, [r1]!
; CHECK-NEXT:  vst1.32 {d{{[0-9]+}}, d{{[0-9]+}}}, [r1]!
; CHECK-NEXT:  vst1.32 {d{{[0-9]+}}, d{{[0-9]+}}}, [r1]!
; CHECK-NEXT:  vst1.64 {d{{[0-9]+}}, d{{[0-9]+}}}, [r1]
; CHECK-NEXT:  mov pc, lr

; This function compiles into DAG with load instruction having negative
; address increment. We can't optimize this now, just test that we don't
; crash and generate something meaningful
define void @load_negative(<4 x i32>* %data, i32* %nn, i32 %ndim)  {
entry:
  %n.vec = and i32 %ndim, -8
  br label %loop

loop:
  %index = phi i32 [ 0, %entry ], [ %index.next, %loop ]
  %vec.phi = phi <4 x i32> [ <i32 1, i32 1, i32 1, i32 1>, %entry ], [ %4, %loop ]
  %offset.idx = or i32 %index, 1
  %0 = getelementptr inbounds i32, i32* %nn, i32 %offset.idx
  %1 = bitcast i32* %0 to <4 x i32>*
  %l1 = load <4 x i32>, <4 x i32>* %1, align 4
  %2 = getelementptr i32, i32* %0, i32 4
  %3 = bitcast i32* %2 to <4 x i32>*
  %l2 = load <4 x i32>, <4 x i32>* %3, align 4
  %4 = mul nsw <4 x i32> %l1, %vec.phi
  %5 = mul nsw <4 x i32> %l2, %vec.phi
  %index.next = add i32 %index, 8
  %6 = icmp eq i32 %index.next, %n.vec
  br i1 %6, label %res, label %loop

res:
  %r = mul nsw <4 x i32> %4, %5
  store <4 x i32> %r, <4 x i32>* %data
  ret void
}

; CHECK:   sub r[[RD:[0-9]+]], {{r[0-9]+}}, #16
; CHECK:   vld1.{{.*}} {d{{[0-9]+}}, d{{[0-9]+}}}, [r[[RD]]]
