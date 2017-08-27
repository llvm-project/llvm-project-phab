; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
;RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=knl | FileCheck %s --check-prefix=KNL
;RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=skx | FileCheck %s --check-prefix=SKX

define i32 @hadd_16(<16 x i32> %x225) {
; KNL-LABEL: hadd_16:
; KNL:       # BB#0:
; KNL-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; KNL-NEXT:    vpaddd %zmm1, %zmm0, %zmm0
; KNL-NEXT:    vphaddd %ymm0, %ymm0, %ymm0
; KNL-NEXT:    vmovd %xmm0, %eax
; KNL-NEXT:    retq
;
; SKX-LABEL: hadd_16:
; SKX:       # BB#0:
; SKX-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SKX-NEXT:    vpaddd %zmm1, %zmm0, %zmm0
; SKX-NEXT:    vphaddd %ymm0, %ymm0, %ymm0
; SKX-NEXT:    vmovd %xmm0, %eax
; SKX-NEXT:    vzeroupper
; SKX-NEXT:    retq
  %x226 = shufflevector <16 x i32> %x225, <16 x i32> undef, <16 x i32> <i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %x227 = add <16 x i32> %x225, %x226
  %x228 = shufflevector <16 x i32> %x227, <16 x i32> undef, <16 x i32> <i32 1, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %x229 = add <16 x i32> %x227, %x228
  %x230 = extractelement <16 x i32> %x229, i32 0
  ret i32 %x230
}

define i32 @hsub_16(<16 x i32> %x225) {
; KNL-LABEL: hsub_16:
; KNL:       # BB#0:
; KNL-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; KNL-NEXT:    vpaddd %zmm1, %zmm0, %zmm0
; KNL-NEXT:    vphsubd %ymm0, %ymm0, %ymm0
; KNL-NEXT:    vmovd %xmm0, %eax
; KNL-NEXT:    retq
;
; SKX-LABEL: hsub_16:
; SKX:       # BB#0:
; SKX-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SKX-NEXT:    vpaddd %zmm1, %zmm0, %zmm0
; SKX-NEXT:    vphsubd %ymm0, %ymm0, %ymm0
; SKX-NEXT:    vmovd %xmm0, %eax
; SKX-NEXT:    vzeroupper
; SKX-NEXT:    retq
  %x226 = shufflevector <16 x i32> %x225, <16 x i32> undef, <16 x i32> <i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %x227 = add <16 x i32> %x225, %x226
  %x228 = shufflevector <16 x i32> %x227, <16 x i32> undef, <16 x i32> <i32 1, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %x229 = sub <16 x i32> %x227, %x228
  %x230 = extractelement <16 x i32> %x229, i32 0
  ret i32 %x230
}

define float @fhadd_16(<16 x float> %x225) {
; KNL-LABEL: fhadd_16:
; KNL:       # BB#0:
; KNL-NEXT:    vpermilpd {{.*#+}} xmm1 = xmm0[1,0]
; KNL-NEXT:    vaddps %zmm1, %zmm0, %zmm0
; KNL-NEXT:    vhaddps %ymm0, %ymm0, %ymm0
; KNL-NEXT:    # kill: %XMM0<def> %XMM0<kill> %ZMM0<kill>
; KNL-NEXT:    retq
;
; SKX-LABEL: fhadd_16:
; SKX:       # BB#0:
; SKX-NEXT:    vpermilpd {{.*#+}} xmm1 = xmm0[1,0]
; SKX-NEXT:    vaddps %zmm1, %zmm0, %zmm0
; SKX-NEXT:    vhaddps %ymm0, %ymm0, %ymm0
; SKX-NEXT:    # kill: %XMM0<def> %XMM0<kill> %ZMM0<kill>
; SKX-NEXT:    vzeroupper
; SKX-NEXT:    retq
  %x226 = shufflevector <16 x float> %x225, <16 x float> undef, <16 x i32> <i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %x227 = fadd <16 x float> %x225, %x226
  %x228 = shufflevector <16 x float> %x227, <16 x float> undef, <16 x i32> <i32 1, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %x229 = fadd <16 x float> %x227, %x228
  %x230 = extractelement <16 x float> %x229, i32 0
  ret float %x230
}

define float @fhsub_16(<16 x float> %x225) {
; KNL-LABEL: fhsub_16:
; KNL:       # BB#0:
; KNL-NEXT:    vpermilpd {{.*#+}} xmm1 = xmm0[1,0]
; KNL-NEXT:    vaddps %zmm1, %zmm0, %zmm0
; KNL-NEXT:    vhsubps %ymm0, %ymm0, %ymm0
; KNL-NEXT:    # kill: %XMM0<def> %XMM0<kill> %ZMM0<kill>
; KNL-NEXT:    retq
;
; SKX-LABEL: fhsub_16:
; SKX:       # BB#0:
; SKX-NEXT:    vpermilpd {{.*#+}} xmm1 = xmm0[1,0]
; SKX-NEXT:    vaddps %zmm1, %zmm0, %zmm0
; SKX-NEXT:    vhsubps %ymm0, %ymm0, %ymm0
; SKX-NEXT:    # kill: %XMM0<def> %XMM0<kill> %ZMM0<kill>
; SKX-NEXT:    vzeroupper
; SKX-NEXT:    retq
  %x226 = shufflevector <16 x float> %x225, <16 x float> undef, <16 x i32> <i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %x227 = fadd <16 x float> %x225, %x226
  %x228 = shufflevector <16 x float> %x227, <16 x float> undef, <16 x i32> <i32 1, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %x229 = fsub <16 x float> %x227, %x228
  %x230 = extractelement <16 x float> %x229, i32 0
  ret float %x230
}

define <16 x i32> @hadd_16_3(<16 x i32> %x225, <16 x i32> %x227) {
; CHECK-LABEL: hadd_16_3:
; CHECK:       # BB#0:
; CHECK-NEXT:    vphaddd %ymm1, %ymm0, %ymm0
; CHECK-NEXT:    retq
; KNL-LABEL: hadd_16_3:
; KNL:       # BB#0:
; KNL-NEXT:    vphaddd %ymm1, %ymm0, %ymm0
; KNL-NEXT:    retq
;
; SKX-LABEL: hadd_16_3:
; SKX:       # BB#0:
; SKX-NEXT:    vphaddd %ymm1, %ymm0, %ymm0
; SKX-NEXT:    retq
  %x226 = shufflevector <16 x i32> %x225, <16 x i32> %x227, <16 x i32> <i32 0, i32 2, i32 16, i32 18
, i32 4, i32 6, i32 20, i32 22, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %x228 = shufflevector <16 x i32> %x225, <16 x i32> %x227, <16 x i32> <i32 1, i32 3, i32 17, i32 19
, i32 5 , i32 7, i32 21,   i32 23, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef,
 i32 undef, i32 undef>
  %x229 = add <16 x i32> %x226, %x228
  ret <16 x i32> %x229
}

define <16 x float> @fhadd_16_3(<16 x float> %x225, <16 x float> %x227) {
; CHECK-LABEL: fhadd_16_3:
; CHECK:       # BB#0:
; CHECK-NEXT:    vhaddps %ymm1, %ymm0, %ymm0
; CHECK-NEXT:    retq
; KNL-LABEL: fhadd_16_3:
; KNL:       # BB#0:
; KNL-NEXT:    vhaddps %ymm1, %ymm0, %ymm0
; KNL-NEXT:    retq
;
; SKX-LABEL: fhadd_16_3:
; SKX:       # BB#0:
; SKX-NEXT:    vhaddps %ymm1, %ymm0, %ymm0
; SKX-NEXT:    retq
  %x226 = shufflevector <16 x float> %x225, <16 x float> %x227, <16 x i32> <i32 0, i32 2, i32 16, i32 18
, i32 4, i32 6, i32 20, i32 22, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %x228 = shufflevector <16 x float> %x225, <16 x float> %x227, <16 x i32> <i32 1, i32 3, i32 17, i32 19
, i32 5 , i32 7, i32 21,   i32 23, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %x229 = fadd <16 x float> %x226, %x228
  ret <16 x float> %x229
}

define <8 x double> @fhadd_16_4(<8 x double> %x225, <8 x double> %x227) {
; CHECK-LABEL: fhadd_16_4:
; CHECK:       # BB#0:
; CHECK-NEXT:    vunpcklpd {{.*#+}} ymm2 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; CHECK-NEXT:    vpermpd {{.*#+}} ymm2 = ymm2[0,2,1,3]
; CHECK-NEXT:    vunpckhpd {{.*#+}} ymm0 = ymm0[1],ymm1[1],ymm0[3],ymm1[3]
; CHECK-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[0,2,1,3]
; CHECK-NEXT:    vaddpd %zmm0, %zmm2, %zmm0
; CHECK-NEXT:    retq
; KNL-LABEL: fhadd_16_4:
; KNL:       # BB#0:
; KNL-NEXT:    vhaddpd %ymm1, %ymm0, %ymm0
; KNL-NEXT:    retq
;
; SKX-LABEL: fhadd_16_4:
; SKX:       # BB#0:
; SKX-NEXT:    vhaddpd %ymm1, %ymm0, %ymm0
; SKX-NEXT:    retq
  %x226 = shufflevector <8 x double> %x225, <8 x double> %x227, <8 x i32> <i32 0, i32 8, i32 2, i32 10, i32 undef, i32 undef, i32 undef, i32 undef>
  %x228 = shufflevector <8 x double> %x225, <8 x double> %x227, <8 x i32> <i32 1, i32 9, i32 3, i32 11, i32 undef ,i32 undef, i32 undef, i32 undef>
  %x229 = fadd <8 x double> %x226, %x228
  ret <8 x double> %x229
}
