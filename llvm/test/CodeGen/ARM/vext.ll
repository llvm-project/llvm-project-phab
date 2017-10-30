; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=arm-eabi -mattr=+neon %s -o - -lower-interleaved-accesses=false | FileCheck %s

define <8 x i8> @test_vextd(<8 x i8>* %A, <8 x i8>* %B) nounwind {
; CHECK-LABEL: test_vextd:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vldr d16, [r1]
; CHECK-NEXT:    vldr d17, [r0]
; CHECK-NEXT:    vext.8 d16, d17, d16, #3
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    mov pc, lr
	%tmp1 = load <8 x i8>, <8 x i8>* %A
	%tmp2 = load <8 x i8>, <8 x i8>* %B
	%tmp3 = shufflevector <8 x i8> %tmp1, <8 x i8> %tmp2, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10>
	ret <8 x i8> %tmp3
}

define <8 x i8> @test_vextRd(<8 x i8>* %A, <8 x i8>* %B) nounwind {
; CHECK-LABEL: test_vextRd:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vldr d16, [r0]
; CHECK-NEXT:    vldr d17, [r1]
; CHECK-NEXT:    vext.8 d16, d17, d16, #5
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    mov pc, lr
	%tmp1 = load <8 x i8>, <8 x i8>* %A
	%tmp2 = load <8 x i8>, <8 x i8>* %B
	%tmp3 = shufflevector <8 x i8> %tmp1, <8 x i8> %tmp2, <8 x i32> <i32 13, i32 14, i32 15, i32 0, i32 1, i32 2, i32 3, i32 4>
	ret <8 x i8> %tmp3
}

define <16 x i8> @test_vextq(<16 x i8>* %A, <16 x i8>* %B) nounwind {
; CHECK-LABEL: test_vextq:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vld1.64 {d16, d17}, [r1]
; CHECK-NEXT:    vld1.64 {d18, d19}, [r0]
; CHECK-NEXT:    vext.8 q8, q9, q8, #3
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    vmov r2, r3, d17
; CHECK-NEXT:    mov pc, lr
	%tmp1 = load <16 x i8>, <16 x i8>* %A
	%tmp2 = load <16 x i8>, <16 x i8>* %B
	%tmp3 = shufflevector <16 x i8> %tmp1, <16 x i8> %tmp2, <16 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18>
	ret <16 x i8> %tmp3
}

define <16 x i8> @test_vextRq(<16 x i8>* %A, <16 x i8>* %B) nounwind {
; CHECK-LABEL: test_vextRq:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vld1.64 {d16, d17}, [r0]
; CHECK-NEXT:    vld1.64 {d18, d19}, [r1]
; CHECK-NEXT:    vext.8 q8, q9, q8, #7
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    vmov r2, r3, d17
; CHECK-NEXT:    mov pc, lr
	%tmp1 = load <16 x i8>, <16 x i8>* %A
	%tmp2 = load <16 x i8>, <16 x i8>* %B
	%tmp3 = shufflevector <16 x i8> %tmp1, <16 x i8> %tmp2, <16 x i32> <i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6>
	ret <16 x i8> %tmp3
}

define <4 x i16> @test_vextd16(<4 x i16>* %A, <4 x i16>* %B) nounwind {
; CHECK-LABEL: test_vextd16:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vldr d16, [r1]
; CHECK-NEXT:    vldr d17, [r0]
; CHECK-NEXT:    vext.16 d16, d17, d16, #3
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    mov pc, lr
	%tmp1 = load <4 x i16>, <4 x i16>* %A
	%tmp2 = load <4 x i16>, <4 x i16>* %B
	%tmp3 = shufflevector <4 x i16> %tmp1, <4 x i16> %tmp2, <4 x i32> <i32 3, i32 4, i32 5, i32 6>
	ret <4 x i16> %tmp3
}

define <4 x i32> @test_vextq32(<4 x i32>* %A, <4 x i32>* %B) nounwind {
; CHECK-LABEL: test_vextq32:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vld1.64 {d16, d17}, [r1]
; CHECK-NEXT:    vld1.64 {d18, d19}, [r0]
; CHECK-NEXT:    vext.32 q8, q9, q8, #3
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    vmov r2, r3, d17
; CHECK-NEXT:    mov pc, lr
	%tmp1 = load <4 x i32>, <4 x i32>* %A
	%tmp2 = load <4 x i32>, <4 x i32>* %B
	%tmp3 = shufflevector <4 x i32> %tmp1, <4 x i32> %tmp2, <4 x i32> <i32 3, i32 4, i32 5, i32 6>
	ret <4 x i32> %tmp3
}

; Undef shuffle indices should not prevent matching to VEXT:

define <8 x i8> @test_vextd_undef(<8 x i8>* %A, <8 x i8>* %B) nounwind {
; CHECK-LABEL: test_vextd_undef:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vldr d16, [r1]
; CHECK-NEXT:    vldr d17, [r0]
; CHECK-NEXT:    vext.8 d16, d17, d16, #3
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    mov pc, lr
	%tmp1 = load <8 x i8>, <8 x i8>* %A
	%tmp2 = load <8 x i8>, <8 x i8>* %B
	%tmp3 = shufflevector <8 x i8> %tmp1, <8 x i8> %tmp2, <8 x i32> <i32 3, i32 undef, i32 undef, i32 6, i32 7, i32 8, i32 9, i32 10>
	ret <8 x i8> %tmp3
}

define <16 x i8> @test_vextRq_undef(<16 x i8>* %A, <16 x i8>* %B) nounwind {
; CHECK-LABEL: test_vextRq_undef:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vld1.64 {d16, d17}, [r0]
; CHECK-NEXT:    vld1.64 {d18, d19}, [r1]
; CHECK-NEXT:    vext.8 q8, q9, q8, #7
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    vmov r2, r3, d17
; CHECK-NEXT:    mov pc, lr
	%tmp1 = load <16 x i8>, <16 x i8>* %A
	%tmp2 = load <16 x i8>, <16 x i8>* %B
	%tmp3 = shufflevector <16 x i8> %tmp1, <16 x i8> %tmp2, <16 x i32> <i32 23, i32 24, i32 25, i32 26, i32 undef, i32 undef, i32 29, i32 30, i32 31, i32 0, i32 1, i32 2, i32 3, i32 4, i32 undef, i32 6>
	ret <16 x i8> %tmp3
}

define <16 x i8> @test_vextq_undef_op2(<16 x i8> %a) nounwind {
; CHECK-LABEL: test_vextq_undef_op2:
; CHECK:       @ BB#0: @ %entry
; CHECK-NEXT:    vmov d17, r2, r3
; CHECK-NEXT:    vmov d16, r0, r1
; CHECK-NEXT:    vext.8 q8, q8, q8, #2
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    vmov r2, r3, d17
; CHECK-NEXT:    mov pc, lr
entry:
  %tmp1 = shufflevector <16 x i8> %a, <16 x i8> undef, <16 x i32> <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 0, i32 1>
  ret <16 x i8> %tmp1
}

define <8 x i8> @test_vextd_undef_op2(<8 x i8> %a) nounwind {
; CHECK-LABEL: test_vextd_undef_op2:
; CHECK:       @ BB#0: @ %entry
; CHECK-NEXT:    vmov d16, r0, r1
; CHECK-NEXT:    vext.8 d16, d16, d16, #2
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    mov pc, lr
entry:
  %tmp1 = shufflevector <8 x i8> %a, <8 x i8> undef, <8 x i32> <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1>
  ret <8 x i8> %tmp1
}


define <16 x i8> @test_vextq_undef_op2_undef(<16 x i8> %a) nounwind {
; CHECK-LABEL: test_vextq_undef_op2_undef:
; CHECK:       @ BB#0: @ %entry
; CHECK-NEXT:    vmov d17, r2, r3
; CHECK-NEXT:    vmov d16, r0, r1
; CHECK-NEXT:    vext.8 q8, q8, q8, #2
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    vmov r2, r3, d17
; CHECK-NEXT:    mov pc, lr
entry:
  %tmp1 = shufflevector <16 x i8> %a, <16 x i8> undef, <16 x i32> <i32 2, i32 3, i32 4, i32 undef, i32 undef, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 0, i32 1>
  ret <16 x i8> %tmp1
}

define <8 x i8> @test_vextd_undef_op2_undef(<8 x i8> %a) nounwind {
; CHECK-LABEL: test_vextd_undef_op2_undef:
; CHECK:       @ BB#0: @ %entry
; CHECK-NEXT:    vmov d16, r0, r1
; CHECK-NEXT:    vext.8 d16, d16, d16, #2
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    mov pc, lr
entry:
  %tmp1 = shufflevector <8 x i8> %a, <8 x i8> undef, <8 x i32> <i32 2, i32 3, i32 undef, i32 undef, i32 6, i32 7, i32 undef, i32 1>
  ret <8 x i8> %tmp1
}

; Tests for ReconstructShuffle function. Indices have to be carefully
; chosen to reach lowering phase as a BUILD_VECTOR.

; One vector needs vext, the other can be handled by extract_subvector
; Also checks interleaving of sources is handled correctly.
; Essence: a vext is used on %A and something saner than stack load/store for final result.
define <4 x i16> @test_interleaved(<8 x i16>* %A, <8 x i16>* %B) nounwind {
; CHECK-LABEL: test_interleaved:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vld1.64 {d16, d17}, [r0]
; CHECK-NEXT:    vext.16 d16, d16, d17, #3
; CHECK-NEXT:    vorr d17, d16, d16
; CHECK-NEXT:    vld1.64 {d18, d19}, [r1]
; CHECK-NEXT:    vuzp.16 d16, d17
; CHECK-NEXT:    vzip.16 d16, d18
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    mov pc, lr
        %tmp1 = load <8 x i16>, <8 x i16>* %A
        %tmp2 = load <8 x i16>, <8 x i16>* %B
        %tmp3 = shufflevector <8 x i16> %tmp1, <8 x i16> %tmp2, <4 x i32> <i32 3, i32 8, i32 5, i32 9>
        ret <4 x i16> %tmp3
}

; An undef in the shuffle list should still be optimizable
define <4 x i16> @test_undef(<8 x i16>* %A, <8 x i16>* %B) nounwind {
; CHECK-LABEL: test_undef:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vldr  d16, [r1]
; CHECK-NEXT:    vldr  d17, [r0, #8]
; CHECK-NEXT:    vzip.16 d17, d16
; CHECK-NEXT:    vmov  r0, r1, d17
; CHECK-NEXT:    mov pc, lr
        %tmp1 = load <8 x i16>, <8 x i16>* %A
        %tmp2 = load <8 x i16>, <8 x i16>* %B
        %tmp3 = shufflevector <8 x i16> %tmp1, <8 x i16> %tmp2, <4 x i32> <i32 undef, i32 8, i32 5, i32 9>
        ret <4 x i16> %tmp3
}

; We should ignore a build_vector with more than two sources.
; Use illegal <32 x i16> type to produce such a shuffle after legalizing types.
; Try to look for fallback to by-element inserts.
define <4 x i16> @test_multisource(<32 x i16>* %B) nounwind {
; CHECK-LABEL: test_multisource:
; CHECK:       @ BB#0:
; CHECK-NEXT:  vld1.16 {d16, d17}, [r0:128]!
; CHECK-NEXT:  vld1.16 {d18, d19}, [r0:128]!
; CHECK-NEXT:  vld1.16 {d20, d21}, [r0:128]!
; CHECK-NEXT:  vorr  d24, d20, d20
; CHECK-NEXT:  vld1.64 {d22, d23}, [r0:128]
; CHECK-NEXT:  vzip.16 d24, d22
; CHECK-NEXT:  vtrn.16 q8, q9
; CHECK-NEXT:  vext.16 d18, d20, d24, #2
; CHECK-NEXT:  vext.16 d16, d18, d16, #2
; CHECK-NEXT:  vext.16 d16, d16, d16, #2
; CHECK-NEXT:  vmov  r0, r1, d16
; CHECK-NEXT:  mov pc, lr
        %tmp1 = load <32 x i16>, <32 x i16>* %B
        %tmp2 = shufflevector <32 x i16> %tmp1, <32 x i16> undef, <4 x i32> <i32 0, i32 8, i32 16, i32 24>
        ret <4 x i16> %tmp2
}

; We don't handle shuffles using more than half of a 128-bit vector.
; Again, test for fallback to by-element inserts.
define <4 x i16> @test_largespan(<8 x i16>* %B) nounwind {
; CHECK-LABEL: test_largespan:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vld1.64 {d16, d17}, [r0]
; CHECK-NEXT:    vorr d18, d16, d16
; CHECK-NEXT:    vuzp.16 d18, d17
; CHECK-NEXT:    vmov r0, r1, d18
; CHECK-NEXT:    mov pc, lr
        %tmp1 = load <8 x i16>, <8 x i16>* %B
        %tmp2 = shufflevector <8 x i16> %tmp1, <8 x i16> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
        ret <4 x i16> %tmp2
}

; The actual shuffle code only handles some cases, make sure we check
; this rather than blindly emitting a VECTOR_SHUFFLE (infinite
; lowering loop can result otherwise).
; (There are probably better ways to lower this shuffle, but it's not
; really important.)
define <8 x i16> @test_illegal(<8 x i16>* %A, <8 x i16>* %B) nounwind {
; CHECK-LABEL: test_illegal:
; CHECK:       @ BB#0:
; CHECK-NEXT:	vld1.64	{d16, d17}, [r0]
; CHECK-NEXT:	vorr	d22, d16, d16
; CHECK-NEXT:	vmov.u16	r0, d16[0]
; CHECK-NEXT:	vorr	d23, d16, d16
; CHECK-NEXT:	vmov.u16	r2, d17[3]
; CHECK-NEXT:	vmov.u16	r3, d17[1]
; CHECK-NEXT:	vld1.64	{d18, d19}, [r1]
; CHECK-NEXT:	vmov.u16	r1, d19[1]
; CHECK-NEXT:	vuzp.16	d22, d23
; CHECK-NEXT:	vuzp.16	d22, d18
; CHECK-NEXT:	vmov.16	d20[0], r0
; CHECK-NEXT:	vmov.16	d20[1], r2
; CHECK-NEXT:	vmov.16	d20[2], r3
; CHECK-NEXT:	vmov.16	d20[3], r1
; CHECK-NEXT:	vext.16	d21, d16, d18, #3
; CHECK-NEXT:	vmov	r0, r1, d20
; CHECK-NEXT:	vmov	r2, r3, d21
; CHECK-NEXT:	mov	pc, lr
       %tmp1 = load <8 x i16>, <8 x i16>* %A
       %tmp2 = load <8 x i16>, <8 x i16>* %B
       %tmp3 = shufflevector <8 x i16> %tmp1, <8 x i16> %tmp2, <8 x i32> <i32 0, i32 7, i32 5, i32 13, i32 3, i32 2, i32 2, i32 9>
       ret <8 x i16> %tmp3
}

; PR11129
; Make sure this doesn't crash
define arm_aapcscc void @test_elem_mismatch(<2 x i64>* nocapture %src, <4 x i16>* nocapture %dest) nounwind {
; CHECK-LABEL: test_elem_mismatch:
; CHECK:       @ BB#0:
; CHECK-NEXT:    vld1.64 {d16, d17}, [r0:128]
; CHECK-NEXT:    vmov.32 r0, d16[0]
; CHECK-NEXT:    vmov.32 r2, d17[0]
; CHECK-NEXT:    vmov.16 d16[0], r0
; CHECK-NEXT:    vmov.16 d16[1], r2
; CHECK-NEXT:    vstr d16, [r1]
; CHECK-NEXT:    mov pc, lr
  %tmp0 = load <2 x i64>, <2 x i64>* %src, align 16
  %tmp1 = bitcast <2 x i64> %tmp0 to <4 x i32>
  %tmp2 = extractelement <4 x i32> %tmp1, i32 0
  %tmp3 = extractelement <4 x i32> %tmp1, i32 2
  %tmp4 = trunc i32 %tmp2 to i16
  %tmp5 = trunc i32 %tmp3 to i16
  %tmp6 = insertelement <4 x i16> undef, i16 %tmp4, i32 0
  %tmp7 = insertelement <4 x i16> %tmp6, i16 %tmp5, i32 1
  store <4 x i16> %tmp7, <4 x i16>* %dest, align 4
  ret void
}

define <4 x i32> @test_reverse_and_extract(<2 x i32>* %A) {
; CHECK-LABEL: test_reverse_and_extract:
; CHECK:       @ BB#0: @ %entry
; CHECK-NEXT:    vldr d16, [r0]
; CHECK-NEXT:    vrev64.32 q9, q8
; CHECK-NEXT:    vext.32 q8, q8, q9, #2
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    vmov r2, r3, d17
; CHECK-NEXT:    mov pc, lr
entry:
	%tmp1 = load <2 x i32>, <2 x i32>* %A
  %0 = shufflevector <2 x i32> %tmp1, <2 x i32> undef, <4 x i32> <i32 undef, i32 undef, i32 1, i32 0>
  ret <4 x i32> %0
}

define <4 x i32> @test_dup_and_extract(<2 x i32>* %A) {
; CHECK-LABEL: test_dup_and_extract:
; CHECK:       @ BB#0: @ %entry
; CHECK-NEXT:    vldr d16, [r0]
; CHECK-NEXT:    vdup.32 q9, d16[0]
; CHECK-NEXT:    vext.32 q8, q9, q8, #2
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    vmov r2, r3, d17
; CHECK-NEXT:    mov pc, lr
entry:
	%tmp1 = load <2 x i32>, <2 x i32>* %A
  %0 = shufflevector <2 x i32> %tmp1, <2 x i32> undef, <4 x i32> <i32 0, i32 0, i32 0, i32 1>
  ret <4 x i32> %0
}

define <4 x i32> @test_zip_and_extract(<2 x i32>* %A) {
; CHECK-LABEL: test_zip_and_extract:
; CHECK:       @ BB#0: @ %entry
; CHECK-NEXT:    vldr d16, [r0]
; CHECK-NEXT:    vorr q9, q8, q8
; CHECK-NEXT:    vorr q10, q8, q8
; CHECK-NEXT:    vzip.32 q9, q10
; CHECK-NEXT:    vext.32 q8, q9, q8, #2
; CHECK-NEXT:    vmov r0, r1, d16
; CHECK-NEXT:    vmov r2, r3, d17
; CHECK-NEXT:    mov pc, lr
entry:
	%tmp1 = load <2 x i32>, <2 x i32>* %A
  %0 = shufflevector <2 x i32> %tmp1, <2 x i32> undef, <4 x i32> <i32 1, i32 1, i32 0, i32 1>
  ret <4 x i32> %0
}
