; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+sse2     | FileCheck %s --check-prefix=SSE --check-prefix=SSE2
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+ssse3    | FileCheck %s --check-prefix=SSE --check-prefix=SSSE3
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+avx      | FileCheck %s --check-prefix=AVX --check-prefix=AVX1
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+avx2     | FileCheck %s --check-prefix=AVX --check-prefix=AVX2
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+avx512vl | FileCheck %s --check-prefix=AVX --check-prefix=AVX512 --check-prefix=AVX512F --check-prefix=AVX512VL
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+avx512vl,+avx512bw | FileCheck %s --check-prefix=AVX --check-prefix=AVX512 --check-prefix=AVX512BW

define <4 x i32> @test_abs_gt_v4i32(<4 x i32> %a) nounwind {
; SSE2-LABEL: test_abs_gt_v4i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    paddd %xmm1, %xmm0
; SSE2-NEXT:    pxor %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_gt_v4i32:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsd %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: test_abs_gt_v4i32:
; AVX:       # BB#0:
; AVX-NEXT:    vpabsd %xmm0, %xmm0
; AVX-NEXT:    retq
  %tmp1neg = sub <4 x i32> zeroinitializer, %a
  %b = icmp sgt <4 x i32> %a, <i32 -1, i32 -1, i32 -1, i32 -1>
  %abs = select <4 x i1> %b, <4 x i32> %a, <4 x i32> %tmp1neg
  ret <4 x i32> %abs
}

define <4 x i32> @test_abs_ge_v4i32(<4 x i32> %a) nounwind {
; SSE2-LABEL: test_abs_ge_v4i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    paddd %xmm1, %xmm0
; SSE2-NEXT:    pxor %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_ge_v4i32:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsd %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: test_abs_ge_v4i32:
; AVX:       # BB#0:
; AVX-NEXT:    vpabsd %xmm0, %xmm0
; AVX-NEXT:    retq
  %tmp1neg = sub <4 x i32> zeroinitializer, %a
  %b = icmp sge <4 x i32> %a, zeroinitializer
  %abs = select <4 x i1> %b, <4 x i32> %a, <4 x i32> %tmp1neg
  ret <4 x i32> %abs
}

define <8 x i16> @test_abs_gt_v8i16(<8 x i16> %a) nounwind {
; SSE2-LABEL: test_abs_gt_v8i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psraw $15, %xmm1
; SSE2-NEXT:    paddw %xmm1, %xmm0
; SSE2-NEXT:    pxor %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_gt_v8i16:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsw %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: test_abs_gt_v8i16:
; AVX:       # BB#0:
; AVX-NEXT:    vpabsw %xmm0, %xmm0
; AVX-NEXT:    retq
  %tmp1neg = sub <8 x i16> zeroinitializer, %a
  %b = icmp sgt <8 x i16> %a, zeroinitializer
  %abs = select <8 x i1> %b, <8 x i16> %a, <8 x i16> %tmp1neg
  ret <8 x i16> %abs
}

define <16 x i8> @test_abs_lt_v16i8(<16 x i8> %a) nounwind {
; SSE2-LABEL: test_abs_lt_v16i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm1, %xmm1
; SSE2-NEXT:    pcmpgtb %xmm0, %xmm1
; SSE2-NEXT:    paddb %xmm1, %xmm0
; SSE2-NEXT:    pxor %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_lt_v16i8:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsb %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: test_abs_lt_v16i8:
; AVX:       # BB#0:
; AVX-NEXT:    vpabsb %xmm0, %xmm0
; AVX-NEXT:    retq
  %tmp1neg = sub <16 x i8> zeroinitializer, %a
  %b = icmp slt <16 x i8> %a, zeroinitializer
  %abs = select <16 x i1> %b, <16 x i8> %tmp1neg, <16 x i8> %a
  ret <16 x i8> %abs
}

define <4 x i32> @test_abs_le_v4i32(<4 x i32> %a) nounwind {
; SSE2-LABEL: test_abs_le_v4i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    paddd %xmm1, %xmm0
; SSE2-NEXT:    pxor %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_le_v4i32:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsd %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: test_abs_le_v4i32:
; AVX:       # BB#0:
; AVX-NEXT:    vpabsd %xmm0, %xmm0
; AVX-NEXT:    retq
  %tmp1neg = sub <4 x i32> zeroinitializer, %a
  %b = icmp sle <4 x i32> %a, zeroinitializer
  %abs = select <4 x i1> %b, <4 x i32> %tmp1neg, <4 x i32> %a
  ret <4 x i32> %abs
}

define <8 x i32> @test_abs_gt_v8i32(<8 x i32> %a) nounwind {
; SSE2-LABEL: test_abs_gt_v8i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    paddd %xmm2, %xmm0
; SSE2-NEXT:    pxor %xmm2, %xmm0
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    paddd %xmm2, %xmm1
; SSE2-NEXT:    pxor %xmm2, %xmm1
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_gt_v8i32:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsd %xmm0, %xmm0
; SSSE3-NEXT:    pabsd %xmm1, %xmm1
; SSSE3-NEXT:    retq
;
; AVX1-LABEL: test_abs_gt_v8i32:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpabsd %xmm0, %xmm1
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpabsd %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_gt_v8i32:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpabsd %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512-LABEL: test_abs_gt_v8i32:
; AVX512:       # BB#0:
; AVX512-NEXT:    vpabsd %ymm0, %ymm0
; AVX512-NEXT:    retq
  %tmp1neg = sub <8 x i32> zeroinitializer, %a
  %b = icmp sgt <8 x i32> %a, <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>
  %abs = select <8 x i1> %b, <8 x i32> %a, <8 x i32> %tmp1neg
  ret <8 x i32> %abs
}

define <8 x i32> @test_abs_ge_v8i32(<8 x i32> %a) nounwind {
; SSE2-LABEL: test_abs_ge_v8i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    paddd %xmm2, %xmm0
; SSE2-NEXT:    pxor %xmm2, %xmm0
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    paddd %xmm2, %xmm1
; SSE2-NEXT:    pxor %xmm2, %xmm1
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_ge_v8i32:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsd %xmm0, %xmm0
; SSSE3-NEXT:    pabsd %xmm1, %xmm1
; SSSE3-NEXT:    retq
;
; AVX1-LABEL: test_abs_ge_v8i32:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpabsd %xmm0, %xmm1
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpabsd %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_ge_v8i32:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpabsd %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512-LABEL: test_abs_ge_v8i32:
; AVX512:       # BB#0:
; AVX512-NEXT:    vpabsd %ymm0, %ymm0
; AVX512-NEXT:    retq
  %tmp1neg = sub <8 x i32> zeroinitializer, %a
  %b = icmp sge <8 x i32> %a, zeroinitializer
  %abs = select <8 x i1> %b, <8 x i32> %a, <8 x i32> %tmp1neg
  ret <8 x i32> %abs
}

define <16 x i16> @test_abs_gt_v16i16(<16 x i16> %a) nounwind {
; SSE2-LABEL: test_abs_gt_v16i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    psraw $15, %xmm2
; SSE2-NEXT:    paddw %xmm2, %xmm0
; SSE2-NEXT:    pxor %xmm2, %xmm0
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psraw $15, %xmm2
; SSE2-NEXT:    paddw %xmm2, %xmm1
; SSE2-NEXT:    pxor %xmm2, %xmm1
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_gt_v16i16:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsw %xmm0, %xmm0
; SSSE3-NEXT:    pabsw %xmm1, %xmm1
; SSSE3-NEXT:    retq
;
; AVX1-LABEL: test_abs_gt_v16i16:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpabsw %xmm0, %xmm1
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpabsw %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_gt_v16i16:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpabsw %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512-LABEL: test_abs_gt_v16i16:
; AVX512:       # BB#0:
; AVX512-NEXT:    vpabsw %ymm0, %ymm0
; AVX512-NEXT:    retq
  %tmp1neg = sub <16 x i16> zeroinitializer, %a
  %b = icmp sgt <16 x i16> %a, zeroinitializer
  %abs = select <16 x i1> %b, <16 x i16> %a, <16 x i16> %tmp1neg
  ret <16 x i16> %abs
}

define <32 x i8> @test_abs_lt_v32i8(<32 x i8> %a) nounwind {
; SSE2-LABEL: test_abs_lt_v32i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm2, %xmm2
; SSE2-NEXT:    pxor %xmm3, %xmm3
; SSE2-NEXT:    pcmpgtb %xmm0, %xmm3
; SSE2-NEXT:    paddb %xmm3, %xmm0
; SSE2-NEXT:    pxor %xmm3, %xmm0
; SSE2-NEXT:    pcmpgtb %xmm1, %xmm2
; SSE2-NEXT:    paddb %xmm2, %xmm1
; SSE2-NEXT:    pxor %xmm2, %xmm1
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_lt_v32i8:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsb %xmm0, %xmm0
; SSSE3-NEXT:    pabsb %xmm1, %xmm1
; SSSE3-NEXT:    retq
;
; AVX1-LABEL: test_abs_lt_v32i8:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpabsb %xmm0, %xmm1
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpabsb %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_lt_v32i8:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpabsb %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512-LABEL: test_abs_lt_v32i8:
; AVX512:       # BB#0:
; AVX512-NEXT:    vpabsb %ymm0, %ymm0
; AVX512-NEXT:    retq
  %tmp1neg = sub <32 x i8> zeroinitializer, %a
  %b = icmp slt <32 x i8> %a, zeroinitializer
  %abs = select <32 x i1> %b, <32 x i8> %tmp1neg, <32 x i8> %a
  ret <32 x i8> %abs
}

define <8 x i32> @test_abs_le_v8i32(<8 x i32> %a) nounwind {
; SSE2-LABEL: test_abs_le_v8i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    paddd %xmm2, %xmm0
; SSE2-NEXT:    pxor %xmm2, %xmm0
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    paddd %xmm2, %xmm1
; SSE2-NEXT:    pxor %xmm2, %xmm1
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_le_v8i32:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsd %xmm0, %xmm0
; SSSE3-NEXT:    pabsd %xmm1, %xmm1
; SSSE3-NEXT:    retq
;
; AVX1-LABEL: test_abs_le_v8i32:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpabsd %xmm0, %xmm1
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpabsd %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_le_v8i32:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpabsd %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512-LABEL: test_abs_le_v8i32:
; AVX512:       # BB#0:
; AVX512-NEXT:    vpabsd %ymm0, %ymm0
; AVX512-NEXT:    retq
  %tmp1neg = sub <8 x i32> zeroinitializer, %a
  %b = icmp sle <8 x i32> %a, zeroinitializer
  %abs = select <8 x i1> %b, <8 x i32> %tmp1neg, <8 x i32> %a
  ret <8 x i32> %abs
}

define <16 x i32> @test_abs_le_16i32(<16 x i32> %a) nounwind {
; SSE2-LABEL: test_abs_le_16i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm4
; SSE2-NEXT:    psrad $31, %xmm4
; SSE2-NEXT:    paddd %xmm4, %xmm0
; SSE2-NEXT:    pxor %xmm4, %xmm0
; SSE2-NEXT:    movdqa %xmm1, %xmm4
; SSE2-NEXT:    psrad $31, %xmm4
; SSE2-NEXT:    paddd %xmm4, %xmm1
; SSE2-NEXT:    pxor %xmm4, %xmm1
; SSE2-NEXT:    movdqa %xmm2, %xmm4
; SSE2-NEXT:    psrad $31, %xmm4
; SSE2-NEXT:    paddd %xmm4, %xmm2
; SSE2-NEXT:    pxor %xmm4, %xmm2
; SSE2-NEXT:    movdqa %xmm3, %xmm4
; SSE2-NEXT:    psrad $31, %xmm4
; SSE2-NEXT:    paddd %xmm4, %xmm3
; SSE2-NEXT:    pxor %xmm4, %xmm3
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_le_16i32:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsd %xmm0, %xmm0
; SSSE3-NEXT:    pabsd %xmm1, %xmm1
; SSSE3-NEXT:    pabsd %xmm2, %xmm2
; SSSE3-NEXT:    pabsd %xmm3, %xmm3
; SSSE3-NEXT:    retq
;
; AVX1-LABEL: test_abs_le_16i32:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpabsd %xmm0, %xmm2
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpabsd %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm2, %ymm0
; AVX1-NEXT:    vpabsd %xmm1, %xmm2
; AVX1-NEXT:    vextractf128 $1, %ymm1, %xmm1
; AVX1-NEXT:    vpabsd %xmm1, %xmm1
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm2, %ymm1
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_le_16i32:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpabsd %ymm0, %ymm0
; AVX2-NEXT:    vpabsd %ymm1, %ymm1
; AVX2-NEXT:    retq
;
; AVX512-LABEL: test_abs_le_16i32:
; AVX512:       # BB#0:
; AVX512-NEXT:    vpabsd %zmm0, %zmm0
; AVX512-NEXT:    retq
  %tmp1neg = sub <16 x i32> zeroinitializer, %a
  %b = icmp sle <16 x i32> %a, zeroinitializer
  %abs = select <16 x i1> %b, <16 x i32> %tmp1neg, <16 x i32> %a
  ret <16 x i32> %abs
}

define <2 x i64> @test_abs_ge_v2i64(<2 x i64> %a) nounwind {
; SSE-LABEL: test_abs_ge_v2i64:
; SSE:       # BB#0:
; SSE-NEXT:    movdqa %xmm0, %xmm1
; SSE-NEXT:    psrad $31, %xmm1
; SSE-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[1,1,3,3]
; SSE-NEXT:    paddq %xmm1, %xmm0
; SSE-NEXT:    pxor %xmm1, %xmm0
; SSE-NEXT:    retq
;
; AVX1-LABEL: test_abs_ge_v2i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpxor %xmm1, %xmm1, %xmm1
; AVX1-NEXT:    vpcmpgtq %xmm0, %xmm1, %xmm1
; AVX1-NEXT:    vpaddq %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpxor %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_ge_v2i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpxor %xmm1, %xmm1, %xmm1
; AVX2-NEXT:    vpcmpgtq %xmm0, %xmm1, %xmm1
; AVX2-NEXT:    vpaddq %xmm1, %xmm0, %xmm0
; AVX2-NEXT:    vpxor %xmm1, %xmm0, %xmm0
; AVX2-NEXT:    retq
;
; AVX512-LABEL: test_abs_ge_v2i64:
; AVX512:       # BB#0:
; AVX512-NEXT:    vpabsq %xmm0, %xmm0
; AVX512-NEXT:    retq
  %tmp1neg = sub <2 x i64> zeroinitializer, %a
  %b = icmp sge <2 x i64> %a, zeroinitializer
  %abs = select <2 x i1> %b, <2 x i64> %a, <2 x i64> %tmp1neg
  ret <2 x i64> %abs
}

define <4 x i64> @test_abs_gt_v4i64(<4 x i64> %a) nounwind {
; SSE-LABEL: test_abs_gt_v4i64:
; SSE:       # BB#0:
; SSE-NEXT:    movdqa %xmm0, %xmm2
; SSE-NEXT:    psrad $31, %xmm2
; SSE-NEXT:    pshufd {{.*#+}} xmm2 = xmm2[1,1,3,3]
; SSE-NEXT:    paddq %xmm2, %xmm0
; SSE-NEXT:    pxor %xmm2, %xmm0
; SSE-NEXT:    movdqa %xmm1, %xmm2
; SSE-NEXT:    psrad $31, %xmm2
; SSE-NEXT:    pshufd {{.*#+}} xmm2 = xmm2[1,1,3,3]
; SSE-NEXT:    paddq %xmm2, %xmm1
; SSE-NEXT:    pxor %xmm2, %xmm1
; SSE-NEXT:    retq
;
; AVX1-LABEL: test_abs_gt_v4i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX1-NEXT:    vpxor %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpcmpgtq %xmm1, %xmm2, %xmm3
; AVX1-NEXT:    vpcmpgtq %xmm0, %xmm2, %xmm2
; AVX1-NEXT:    vinsertf128 $1, %xmm3, %ymm2, %ymm4
; AVX1-NEXT:    vpaddq %xmm3, %xmm1, %xmm1
; AVX1-NEXT:    vpaddq %xmm2, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    vxorps %ymm4, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_gt_v4i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpxor %ymm1, %ymm1, %ymm1
; AVX2-NEXT:    vpcmpgtq %ymm0, %ymm1, %ymm1
; AVX2-NEXT:    vpaddq %ymm1, %ymm0, %ymm0
; AVX2-NEXT:    vpxor %ymm1, %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512-LABEL: test_abs_gt_v4i64:
; AVX512:       # BB#0:
; AVX512-NEXT:    vpabsq %ymm0, %ymm0
; AVX512-NEXT:    retq
  %tmp1neg = sub <4 x i64> zeroinitializer, %a
  %b = icmp sgt <4 x i64> %a, <i64 -1, i64 -1, i64 -1, i64 -1>
  %abs = select <4 x i1> %b, <4 x i64> %a, <4 x i64> %tmp1neg
  ret <4 x i64> %abs
}

define <8 x i64> @test_abs_le_v8i64(<8 x i64> %a) nounwind {
; SSE-LABEL: test_abs_le_v8i64:
; SSE:       # BB#0:
; SSE-NEXT:    movdqa %xmm0, %xmm4
; SSE-NEXT:    psrad $31, %xmm4
; SSE-NEXT:    pshufd {{.*#+}} xmm4 = xmm4[1,1,3,3]
; SSE-NEXT:    paddq %xmm4, %xmm0
; SSE-NEXT:    pxor %xmm4, %xmm0
; SSE-NEXT:    movdqa %xmm1, %xmm4
; SSE-NEXT:    psrad $31, %xmm4
; SSE-NEXT:    pshufd {{.*#+}} xmm4 = xmm4[1,1,3,3]
; SSE-NEXT:    paddq %xmm4, %xmm1
; SSE-NEXT:    pxor %xmm4, %xmm1
; SSE-NEXT:    movdqa %xmm2, %xmm4
; SSE-NEXT:    psrad $31, %xmm4
; SSE-NEXT:    pshufd {{.*#+}} xmm4 = xmm4[1,1,3,3]
; SSE-NEXT:    paddq %xmm4, %xmm2
; SSE-NEXT:    pxor %xmm4, %xmm2
; SSE-NEXT:    movdqa %xmm3, %xmm4
; SSE-NEXT:    psrad $31, %xmm4
; SSE-NEXT:    pshufd {{.*#+}} xmm4 = xmm4[1,1,3,3]
; SSE-NEXT:    paddq %xmm4, %xmm3
; SSE-NEXT:    pxor %xmm4, %xmm3
; SSE-NEXT:    retq
;
; AVX1-LABEL: test_abs_le_v8i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm2
; AVX1-NEXT:    vpxor %xmm3, %xmm3, %xmm3
; AVX1-NEXT:    vpcmpgtq %xmm2, %xmm3, %xmm4
; AVX1-NEXT:    vpcmpgtq %xmm0, %xmm3, %xmm5
; AVX1-NEXT:    vinsertf128 $1, %xmm4, %ymm5, %ymm6
; AVX1-NEXT:    vpaddq %xmm4, %xmm2, %xmm2
; AVX1-NEXT:    vpaddq %xmm5, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    vxorps %ymm6, %ymm0, %ymm0
; AVX1-NEXT:    vextractf128 $1, %ymm1, %xmm2
; AVX1-NEXT:    vpcmpgtq %xmm2, %xmm3, %xmm4
; AVX1-NEXT:    vpcmpgtq %xmm1, %xmm3, %xmm3
; AVX1-NEXT:    vinsertf128 $1, %xmm4, %ymm3, %ymm5
; AVX1-NEXT:    vpaddq %xmm4, %xmm2, %xmm2
; AVX1-NEXT:    vpaddq %xmm3, %xmm1, %xmm1
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm1, %ymm1
; AVX1-NEXT:    vxorps %ymm5, %ymm1, %ymm1
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_le_v8i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpxor %ymm2, %ymm2, %ymm2
; AVX2-NEXT:    vpcmpgtq %ymm0, %ymm2, %ymm3
; AVX2-NEXT:    vpaddq %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpxor %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpcmpgtq %ymm1, %ymm2, %ymm2
; AVX2-NEXT:    vpaddq %ymm2, %ymm1, %ymm1
; AVX2-NEXT:    vpxor %ymm2, %ymm1, %ymm1
; AVX2-NEXT:    retq
;
; AVX512-LABEL: test_abs_le_v8i64:
; AVX512:       # BB#0:
; AVX512-NEXT:    vpabsq %zmm0, %zmm0
; AVX512-NEXT:    retq
  %tmp1neg = sub <8 x i64> zeroinitializer, %a
  %b = icmp sle <8 x i64> %a, zeroinitializer
  %abs = select <8 x i1> %b, <8 x i64> %tmp1neg, <8 x i64> %a
  ret <8 x i64> %abs
}

define <8 x i64> @test_abs_le_v8i64_fold(<8 x i64>* %a.ptr) nounwind {
; SSE-LABEL: test_abs_le_v8i64_fold:
; SSE:       # BB#0:
; SSE-NEXT:    movdqu (%rdi), %xmm0
; SSE-NEXT:    movdqu 16(%rdi), %xmm1
; SSE-NEXT:    movdqu 32(%rdi), %xmm2
; SSE-NEXT:    movdqu 48(%rdi), %xmm3
; SSE-NEXT:    movdqa %xmm0, %xmm4
; SSE-NEXT:    psrad $31, %xmm4
; SSE-NEXT:    pshufd {{.*#+}} xmm4 = xmm4[1,1,3,3]
; SSE-NEXT:    paddq %xmm4, %xmm0
; SSE-NEXT:    pxor %xmm4, %xmm0
; SSE-NEXT:    movdqa %xmm1, %xmm4
; SSE-NEXT:    psrad $31, %xmm4
; SSE-NEXT:    pshufd {{.*#+}} xmm4 = xmm4[1,1,3,3]
; SSE-NEXT:    paddq %xmm4, %xmm1
; SSE-NEXT:    pxor %xmm4, %xmm1
; SSE-NEXT:    movdqa %xmm2, %xmm4
; SSE-NEXT:    psrad $31, %xmm4
; SSE-NEXT:    pshufd {{.*#+}} xmm4 = xmm4[1,1,3,3]
; SSE-NEXT:    paddq %xmm4, %xmm2
; SSE-NEXT:    pxor %xmm4, %xmm2
; SSE-NEXT:    movdqa %xmm3, %xmm4
; SSE-NEXT:    psrad $31, %xmm4
; SSE-NEXT:    pshufd {{.*#+}} xmm4 = xmm4[1,1,3,3]
; SSE-NEXT:    paddq %xmm4, %xmm3
; SSE-NEXT:    pxor %xmm4, %xmm3
; SSE-NEXT:    retq
;
; AVX1-LABEL: test_abs_le_v8i64_fold:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovdqu (%rdi), %xmm0
; AVX1-NEXT:    vmovdqu 16(%rdi), %xmm1
; AVX1-NEXT:    vmovdqu 32(%rdi), %xmm2
; AVX1-NEXT:    vmovdqu 48(%rdi), %xmm3
; AVX1-NEXT:    vpxor %xmm4, %xmm4, %xmm4
; AVX1-NEXT:    vpcmpgtq %xmm1, %xmm4, %xmm5
; AVX1-NEXT:    vpcmpgtq %xmm0, %xmm4, %xmm6
; AVX1-NEXT:    vinsertf128 $1, %xmm5, %ymm6, %ymm7
; AVX1-NEXT:    vpaddq %xmm5, %xmm1, %xmm1
; AVX1-NEXT:    vpaddq %xmm6, %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    vxorps %ymm7, %ymm0, %ymm0
; AVX1-NEXT:    vpcmpgtq %xmm3, %xmm4, %xmm1
; AVX1-NEXT:    vpcmpgtq %xmm2, %xmm4, %xmm4
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm4, %ymm5
; AVX1-NEXT:    vpaddq %xmm1, %xmm3, %xmm1
; AVX1-NEXT:    vpaddq %xmm4, %xmm2, %xmm2
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm2, %ymm1
; AVX1-NEXT:    vxorps %ymm5, %ymm1, %ymm1
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_le_v8i64_fold:
; AVX2:       # BB#0:
; AVX2-NEXT:    vmovdqu (%rdi), %ymm0
; AVX2-NEXT:    vmovdqu 32(%rdi), %ymm1
; AVX2-NEXT:    vpxor %ymm2, %ymm2, %ymm2
; AVX2-NEXT:    vpcmpgtq %ymm0, %ymm2, %ymm3
; AVX2-NEXT:    vpaddq %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpxor %ymm3, %ymm0, %ymm0
; AVX2-NEXT:    vpcmpgtq %ymm1, %ymm2, %ymm2
; AVX2-NEXT:    vpaddq %ymm2, %ymm1, %ymm1
; AVX2-NEXT:    vpxor %ymm2, %ymm1, %ymm1
; AVX2-NEXT:    retq
;
; AVX512-LABEL: test_abs_le_v8i64_fold:
; AVX512:       # BB#0:
; AVX512-NEXT:    vpabsq (%rdi), %zmm0
; AVX512-NEXT:    retq
  %a = load <8 x i64>, <8 x i64>* %a.ptr, align 8
  %tmp1neg = sub <8 x i64> zeroinitializer, %a
  %b = icmp sle <8 x i64> %a, zeroinitializer
  %abs = select <8 x i1> %b, <8 x i64> %tmp1neg, <8 x i64> %a
  ret <8 x i64> %abs
}

define <64 x i8> @test_abs_lt_v64i8(<64 x i8> %a) nounwind {
; SSE2-LABEL: test_abs_lt_v64i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm4, %xmm4
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtb %xmm0, %xmm5
; SSE2-NEXT:    paddb %xmm5, %xmm0
; SSE2-NEXT:    pxor %xmm5, %xmm0
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtb %xmm1, %xmm5
; SSE2-NEXT:    paddb %xmm5, %xmm1
; SSE2-NEXT:    pxor %xmm5, %xmm1
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtb %xmm2, %xmm5
; SSE2-NEXT:    paddb %xmm5, %xmm2
; SSE2-NEXT:    pxor %xmm5, %xmm2
; SSE2-NEXT:    pcmpgtb %xmm3, %xmm4
; SSE2-NEXT:    paddb %xmm4, %xmm3
; SSE2-NEXT:    pxor %xmm4, %xmm3
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_lt_v64i8:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsb %xmm0, %xmm0
; SSSE3-NEXT:    pabsb %xmm1, %xmm1
; SSSE3-NEXT:    pabsb %xmm2, %xmm2
; SSSE3-NEXT:    pabsb %xmm3, %xmm3
; SSSE3-NEXT:    retq
;
; AVX1-LABEL: test_abs_lt_v64i8:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpabsb %xmm0, %xmm2
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpabsb %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm2, %ymm0
; AVX1-NEXT:    vpabsb %xmm1, %xmm2
; AVX1-NEXT:    vextractf128 $1, %ymm1, %xmm1
; AVX1-NEXT:    vpabsb %xmm1, %xmm1
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm2, %ymm1
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_lt_v64i8:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpabsb %ymm0, %ymm0
; AVX2-NEXT:    vpabsb %ymm1, %ymm1
; AVX2-NEXT:    retq
;
; AVX512F-LABEL: test_abs_lt_v64i8:
; AVX512F:       # BB#0:
; AVX512F-NEXT:    vpabsb %ymm0, %ymm0
; AVX512F-NEXT:    vpabsb %ymm1, %ymm1
; AVX512F-NEXT:    retq
;
; AVX512BW-LABEL: test_abs_lt_v64i8:
; AVX512BW:       # BB#0:
; AVX512BW-NEXT:    vpabsb %zmm0, %zmm0
; AVX512BW-NEXT:    retq
  %tmp1neg = sub <64 x i8> zeroinitializer, %a
  %b = icmp slt <64 x i8> %a, zeroinitializer
  %abs = select <64 x i1> %b, <64 x i8> %tmp1neg, <64 x i8> %a
  ret <64 x i8> %abs
}

define <32 x i16> @test_abs_gt_v32i16(<32 x i16> %a) nounwind {
; SSE2-LABEL: test_abs_gt_v32i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm4
; SSE2-NEXT:    psraw $15, %xmm4
; SSE2-NEXT:    paddw %xmm4, %xmm0
; SSE2-NEXT:    pxor %xmm4, %xmm0
; SSE2-NEXT:    movdqa %xmm1, %xmm4
; SSE2-NEXT:    psraw $15, %xmm4
; SSE2-NEXT:    paddw %xmm4, %xmm1
; SSE2-NEXT:    pxor %xmm4, %xmm1
; SSE2-NEXT:    movdqa %xmm2, %xmm4
; SSE2-NEXT:    psraw $15, %xmm4
; SSE2-NEXT:    paddw %xmm4, %xmm2
; SSE2-NEXT:    pxor %xmm4, %xmm2
; SSE2-NEXT:    movdqa %xmm3, %xmm4
; SSE2-NEXT:    psraw $15, %xmm4
; SSE2-NEXT:    paddw %xmm4, %xmm3
; SSE2-NEXT:    pxor %xmm4, %xmm3
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: test_abs_gt_v32i16:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pabsw %xmm0, %xmm0
; SSSE3-NEXT:    pabsw %xmm1, %xmm1
; SSSE3-NEXT:    pabsw %xmm2, %xmm2
; SSSE3-NEXT:    pabsw %xmm3, %xmm3
; SSSE3-NEXT:    retq
;
; AVX1-LABEL: test_abs_gt_v32i16:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpabsw %xmm0, %xmm2
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpabsw %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm2, %ymm0
; AVX1-NEXT:    vpabsw %xmm1, %xmm2
; AVX1-NEXT:    vextractf128 $1, %ymm1, %xmm1
; AVX1-NEXT:    vpabsw %xmm1, %xmm1
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm2, %ymm1
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_abs_gt_v32i16:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpabsw %ymm0, %ymm0
; AVX2-NEXT:    vpabsw %ymm1, %ymm1
; AVX2-NEXT:    retq
;
; AVX512F-LABEL: test_abs_gt_v32i16:
; AVX512F:       # BB#0:
; AVX512F-NEXT:    vpabsw %ymm0, %ymm0
; AVX512F-NEXT:    vpabsw %ymm1, %ymm1
; AVX512F-NEXT:    retq
;
; AVX512BW-LABEL: test_abs_gt_v32i16:
; AVX512BW:       # BB#0:
; AVX512BW-NEXT:    vpabsw %zmm0, %zmm0
; AVX512BW-NEXT:    retq
  %tmp1neg = sub <32 x i16> zeroinitializer, %a
  %b = icmp sgt <32 x i16> %a, zeroinitializer
  %abs = select <32 x i1> %b, <32 x i16> %a, <32 x i16> %tmp1neg
  ret <32 x i16> %abs
}
