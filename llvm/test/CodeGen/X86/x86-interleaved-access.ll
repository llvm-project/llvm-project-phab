; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: llc -mtriple=x86_64-pc-linux  -mattr=+avx < %s | FileCheck %s --check-prefix=AVX --check-prefix=AVX1
; RUN: llc -mtriple=x86_64-pc-linux  -mattr=+avx2 < %s | FileCheck %s --check-prefix=AVX --check-prefix=AVX2

define <4 x double> @load_factorf64_4(<16 x double>* %ptr) {
; AVX-LABEL: load_factorf64_4:
; AVX:       # BB#0:
; AVX-NEXT:    vmovupd (%rdi), %ymm0
; AVX-NEXT:    vmovupd 32(%rdi), %ymm1
; AVX-NEXT:    vmovupd 64(%rdi), %ymm2
; AVX-NEXT:    vmovupd 96(%rdi), %ymm3
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm4 = ymm0[0,1],ymm2[0,1]
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm5 = ymm1[0,1],ymm3[0,1]
; AVX-NEXT:    vhaddpd %ymm5, %ymm4, %ymm4
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3],ymm2[2,3]
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm1 = ymm1[2,3],ymm3[2,3]
; AVX-NEXT:    vunpcklpd {{.*#+}} ymm2 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; AVX-NEXT:    vaddpd %ymm2, %ymm4, %ymm2
; AVX-NEXT:    vunpckhpd {{.*#+}} ymm0 = ymm0[1],ymm1[1],ymm0[3],ymm1[3]
; AVX-NEXT:    vaddpd %ymm0, %ymm2, %ymm0
; AVX-NEXT:    retq
  %wide.vec = load <16 x double>, <16 x double>* %ptr, align 16
  %strided.v0 = shufflevector <16 x double> %wide.vec, <16 x double> undef, <4 x i32> <i32 0, i32 4, i32 8, i32 12>
  %strided.v1 = shufflevector <16 x double> %wide.vec, <16 x double> undef, <4 x i32> <i32 1, i32 5, i32 9, i32 13>
  %strided.v2 = shufflevector <16 x double> %wide.vec, <16 x double> undef, <4 x i32> <i32 2, i32 6, i32 10, i32 14>
  %strided.v3 = shufflevector <16 x double> %wide.vec, <16 x double> undef, <4 x i32> <i32 3, i32 7, i32 11, i32 15>
  %add1 = fadd <4 x double> %strided.v0, %strided.v1
  %add2 = fadd <4 x double> %add1, %strided.v2
  %add3 = fadd <4 x double> %add2, %strided.v3
  ret <4 x double> %add3
}

define <4 x double> @load_factorf64_2(<16 x double>* %ptr) {
; AVX-LABEL: load_factorf64_2:
; AVX:       # BB#0:
; AVX-NEXT:    vmovupd (%rdi), %ymm0
; AVX-NEXT:    vmovupd 32(%rdi), %ymm1
; AVX-NEXT:    vmovupd 64(%rdi), %ymm2
; AVX-NEXT:    vmovupd 96(%rdi), %ymm3
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm4 = ymm0[0,1],ymm2[0,1]
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm5 = ymm1[0,1],ymm3[0,1]
; AVX-NEXT:    vunpcklpd {{.*#+}} ymm4 = ymm4[0],ymm5[0],ymm4[2],ymm5[2]
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3],ymm2[2,3]
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm1 = ymm1[2,3],ymm3[2,3]
; AVX-NEXT:    vunpckhpd {{.*#+}} ymm0 = ymm0[1],ymm1[1],ymm0[3],ymm1[3]
; AVX-NEXT:    vmulpd %ymm0, %ymm4, %ymm0
; AVX-NEXT:    retq
  %wide.vec = load <16 x double>, <16 x double>* %ptr, align 16
  %strided.v0 = shufflevector <16 x double> %wide.vec, <16 x double> undef, <4 x i32> <i32 0, i32 4, i32 8, i32 12>
  %strided.v3 = shufflevector <16 x double> %wide.vec, <16 x double> undef, <4 x i32> <i32 3, i32 7, i32 11, i32 15>
  %mul = fmul <4 x double> %strided.v0, %strided.v3
  ret <4 x double> %mul
}

define <4 x double> @load_factorf64_1(<16 x double>* %ptr) {
; AVX-LABEL: load_factorf64_1:
; AVX:       # BB#0:
; AVX-NEXT:    vmovupd (%rdi), %ymm0
; AVX-NEXT:    vmovupd 32(%rdi), %ymm1
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[0,1],mem[0,1]
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm1 = ymm1[0,1],mem[0,1]
; AVX-NEXT:    vunpcklpd {{.*#+}} ymm0 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; AVX-NEXT:    vmulpd %ymm0, %ymm0, %ymm0
; AVX-NEXT:    retq
  %wide.vec = load <16 x double>, <16 x double>* %ptr, align 16
  %strided.v0 = shufflevector <16 x double> %wide.vec, <16 x double> undef, <4 x i32> <i32 0, i32 4, i32 8, i32 12>
  %strided.v3 = shufflevector <16 x double> %wide.vec, <16 x double> undef, <4 x i32> <i32 0, i32 4, i32 8, i32 12>
  %mul = fmul <4 x double> %strided.v0, %strided.v3
  ret <4 x double> %mul
}

define <4 x i64> @load_factori64_4(<16 x i64>* %ptr) {
; AVX1-LABEL: load_factori64_4:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovupd (%rdi), %ymm0
; AVX1-NEXT:    vmovupd 32(%rdi), %ymm1
; AVX1-NEXT:    vmovupd 64(%rdi), %ymm2
; AVX1-NEXT:    vmovupd 96(%rdi), %ymm3
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm4 = ymm0[0,1],ymm2[0,1]
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm5 = ymm1[0,1],ymm3[0,1]
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3],ymm2[2,3]
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm1 = ymm1[2,3],ymm3[2,3]
; AVX1-NEXT:    vunpcklpd {{.*#+}} ymm2 = ymm4[0],ymm5[0],ymm4[2],ymm5[2]
; AVX1-NEXT:    vunpcklpd {{.*#+}} ymm3 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; AVX1-NEXT:    vunpckhpd {{.*#+}} ymm4 = ymm4[1],ymm5[1],ymm4[3],ymm5[3]
; AVX1-NEXT:    vunpckhpd {{.*#+}} ymm0 = ymm0[1],ymm1[1],ymm0[3],ymm1[3]
; AVX1-NEXT:    vextractf128 $1, %ymm4, %xmm1
; AVX1-NEXT:    vextractf128 $1, %ymm2, %xmm5
; AVX1-NEXT:    vpaddq %xmm3, %xmm4, %xmm4
; AVX1-NEXT:    vextractf128 $1, %ymm3, %xmm3
; AVX1-NEXT:    vpaddq %xmm3, %xmm1, %xmm1
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm3
; AVX1-NEXT:    vpaddq %xmm3, %xmm1, %xmm1
; AVX1-NEXT:    vpaddq %xmm1, %xmm5, %xmm1
; AVX1-NEXT:    vpaddq %xmm0, %xmm4, %xmm0
; AVX1-NEXT:    vpaddq %xmm0, %xmm2, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: load_factori64_4:
; AVX2:       # BB#0:
; AVX2-NEXT:    vmovdqu (%rdi), %ymm0
; AVX2-NEXT:    vmovdqu 32(%rdi), %ymm1
; AVX2-NEXT:    vmovdqu 64(%rdi), %ymm2
; AVX2-NEXT:    vmovdqu 96(%rdi), %ymm3
; AVX2-NEXT:    vperm2i128 {{.*#+}} ymm4 = ymm0[0,1],ymm2[0,1]
; AVX2-NEXT:    vperm2i128 {{.*#+}} ymm5 = ymm1[0,1],ymm3[0,1]
; AVX2-NEXT:    vperm2i128 {{.*#+}} ymm0 = ymm0[2,3],ymm2[2,3]
; AVX2-NEXT:    vperm2i128 {{.*#+}} ymm1 = ymm1[2,3],ymm3[2,3]
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} ymm2 = ymm4[0],ymm5[0],ymm4[2],ymm5[2]
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} ymm3 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; AVX2-NEXT:    vpunpckhqdq {{.*#+}} ymm4 = ymm4[1],ymm5[1],ymm4[3],ymm5[3]
; AVX2-NEXT:    vpaddq %ymm3, %ymm4, %ymm3
; AVX2-NEXT:    vpunpckhqdq {{.*#+}} ymm0 = ymm0[1],ymm1[1],ymm0[3],ymm1[3]
; AVX2-NEXT:    vpaddq %ymm0, %ymm3, %ymm0
; AVX2-NEXT:    vpaddq %ymm0, %ymm2, %ymm0
; AVX2-NEXT:    retq
  %wide.vec = load <16 x i64>, <16 x i64>* %ptr, align 16
  %strided.v0 = shufflevector <16 x i64> %wide.vec, <16 x i64> undef, <4 x i32> <i32 0, i32 4, i32 8, i32 12>
  %strided.v1 = shufflevector <16 x i64> %wide.vec, <16 x i64> undef, <4 x i32> <i32 1, i32 5, i32 9, i32 13>
  %strided.v2 = shufflevector <16 x i64> %wide.vec, <16 x i64> undef, <4 x i32> <i32 2, i32 6, i32 10, i32 14>
  %strided.v3 = shufflevector <16 x i64> %wide.vec, <16 x i64> undef, <4 x i32> <i32 3, i32 7, i32 11, i32 15>
  %add1 = add <4 x i64> %strided.v0, %strided.v1
  %add2 = add <4 x i64> %add1, %strided.v2
  %add3 = add <4 x i64> %add2, %strided.v3
  ret <4 x i64> %add3
}

define void @store_factorf64_4(<16 x double>* %ptr, <4 x double> %v0, <4 x double> %v1, <4 x double> %v2, <4 x double> %v3) {
; AVX-LABEL: store_factorf64_4:
; AVX:       # BB#0:
; AVX-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm4
; AVX-NEXT:    vinsertf128 $1, %xmm3, %ymm1, %ymm5
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3],ymm2[2,3]
; AVX-NEXT:    vperm2f128 {{.*#+}} ymm1 = ymm1[2,3],ymm3[2,3]
; AVX-NEXT:    vunpcklpd {{.*#+}} ymm2 = ymm4[0],ymm5[0],ymm4[2],ymm5[2]
; AVX-NEXT:    vunpcklpd {{.*#+}} ymm3 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; AVX-NEXT:    vunpckhpd {{.*#+}} ymm4 = ymm4[1],ymm5[1],ymm4[3],ymm5[3]
; AVX-NEXT:    vunpckhpd {{.*#+}} ymm0 = ymm0[1],ymm1[1],ymm0[3],ymm1[3]
; AVX-NEXT:    vmovupd %ymm0, 96(%rdi)
; AVX-NEXT:    vmovupd %ymm3, 64(%rdi)
; AVX-NEXT:    vmovupd %ymm4, 32(%rdi)
; AVX-NEXT:    vmovupd %ymm2, (%rdi)
; AVX-NEXT:    vzeroupper
; AVX-NEXT:    retq
  %s0 = shufflevector <4 x double> %v0, <4 x double> %v1, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %s1 = shufflevector <4 x double> %v2, <4 x double> %v3, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %interleaved.vec = shufflevector <8 x double> %s0, <8 x double> %s1, <16 x i32> <i32 0, i32 4, i32 8, i32 12, i32 1, i32 5, i32 9, i32 13, i32 2, i32 6, i32 10, i32 14, i32 3, i32 7, i32 11, i32 15>
  store <16 x double> %interleaved.vec, <16 x double>* %ptr, align 16
  ret void
}

define void @store_factori64_4(<16 x i64>* %ptr, <4 x i64> %v0, <4 x i64> %v1, <4 x i64> %v2, <4 x i64> %v3) {
; AVX1-LABEL: store_factori64_4:
; AVX1:       # BB#0:
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm4
; AVX1-NEXT:    vinsertf128 $1, %xmm3, %ymm1, %ymm5
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3],ymm2[2,3]
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm1 = ymm1[2,3],ymm3[2,3]
; AVX1-NEXT:    vunpcklpd {{.*#+}} ymm2 = ymm4[0],ymm5[0],ymm4[2],ymm5[2]
; AVX1-NEXT:    vunpcklpd {{.*#+}} ymm3 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; AVX1-NEXT:    vunpckhpd {{.*#+}} ymm4 = ymm4[1],ymm5[1],ymm4[3],ymm5[3]
; AVX1-NEXT:    vunpckhpd {{.*#+}} ymm0 = ymm0[1],ymm1[1],ymm0[3],ymm1[3]
; AVX1-NEXT:    vmovupd %ymm0, 96(%rdi)
; AVX1-NEXT:    vmovupd %ymm3, 64(%rdi)
; AVX1-NEXT:    vmovupd %ymm4, 32(%rdi)
; AVX1-NEXT:    vmovupd %ymm2, (%rdi)
; AVX1-NEXT:    vzeroupper
; AVX1-NEXT:    retq
;
; AVX2-LABEL: store_factori64_4:
; AVX2:       # BB#0:
; AVX2-NEXT:    vinserti128 $1, %xmm2, %ymm0, %ymm4
; AVX2-NEXT:    vinserti128 $1, %xmm3, %ymm1, %ymm5
; AVX2-NEXT:    vperm2i128 {{.*#+}} ymm0 = ymm0[2,3],ymm2[2,3]
; AVX2-NEXT:    vperm2i128 {{.*#+}} ymm1 = ymm1[2,3],ymm3[2,3]
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} ymm2 = ymm4[0],ymm5[0],ymm4[2],ymm5[2]
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} ymm3 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; AVX2-NEXT:    vpunpckhqdq {{.*#+}} ymm4 = ymm4[1],ymm5[1],ymm4[3],ymm5[3]
; AVX2-NEXT:    vpunpckhqdq {{.*#+}} ymm0 = ymm0[1],ymm1[1],ymm0[3],ymm1[3]
; AVX2-NEXT:    vmovdqu %ymm0, 96(%rdi)
; AVX2-NEXT:    vmovdqu %ymm3, 64(%rdi)
; AVX2-NEXT:    vmovdqu %ymm4, 32(%rdi)
; AVX2-NEXT:    vmovdqu %ymm2, (%rdi)
; AVX2-NEXT:    vzeroupper
; AVX2-NEXT:    retq
  %s0 = shufflevector <4 x i64> %v0, <4 x i64> %v1, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %s1 = shufflevector <4 x i64> %v2, <4 x i64> %v3, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %interleaved.vec = shufflevector <8 x i64> %s0, <8 x i64> %s1, <16 x i32> <i32 0, i32 4, i32 8, i32 12, i32 1, i32 5, i32 9, i32 13, i32 2, i32 6, i32 10, i32 14, i32 3, i32 7, i32 11, i32 15>
  store <16 x i64> %interleaved.vec, <16 x i64>* %ptr, align 16
  ret void
}


define void @interleaved_store_vf32_i8_stride4(<32 x i8> %x1, <32 x i8> %x2, <32 x i8> %x3, <32 x i8> %x4, <128 x i8>* %p) {
; AVX2-LABEL: interleaved_store_vf32_i8_stride4:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpunpcklbw {{.*#+}} ymm4 = ymm0[0],ymm1[0],ymm0[1],ymm1[1],ymm0[2],ymm1[2],ymm0[3],ymm1[3],ymm0[4],ymm1[4],ymm0[5],ymm1[5],ymm0[6],ymm1[6],ymm0[7],ymm1[7],ymm0[16],ymm1[16],ymm0[17],ymm1[17],ymm0[18],ymm1[18],ymm0[19],ymm1[19],ymm0[20],ymm1[20],ymm0[21],ymm1[21],ymm0[22],ymm1[22],ymm0[23],ymm1[23]
; AVX2-NEXT:    vpunpckhbw {{.*#+}} ymm0 = ymm0[8],ymm1[8],ymm0[9],ymm1[9],ymm0[10],ymm1[10],ymm0[11],ymm1[11],ymm0[12],ymm1[12],ymm0[13],ymm1[13],ymm0[14],ymm1[14],ymm0[15],ymm1[15],ymm0[24],ymm1[24],ymm0[25],ymm1[25],ymm0[26],ymm1[26],ymm0[27],ymm1[27],ymm0[28],ymm1[28],ymm0[29],ymm1[29],ymm0[30],ymm1[30],ymm0[31],ymm1[31]
; AVX2-NEXT:    vpunpcklbw {{.*#+}} ymm1 = ymm2[0],ymm3[0],ymm2[1],ymm3[1],ymm2[2],ymm3[2],ymm2[3],ymm3[3],ymm2[4],ymm3[4],ymm2[5],ymm3[5],ymm2[6],ymm3[6],ymm2[7],ymm3[7],ymm2[16],ymm3[16],ymm2[17],ymm3[17],ymm2[18],ymm3[18],ymm2[19],ymm3[19],ymm2[20],ymm3[20],ymm2[21],ymm3[21],ymm2[22],ymm3[22],ymm2[23],ymm3[23]
; AVX2-NEXT:    vpunpckhbw {{.*#+}} ymm2 = ymm2[8],ymm3[8],ymm2[9],ymm3[9],ymm2[10],ymm3[10],ymm2[11],ymm3[11],ymm2[12],ymm3[12],ymm2[13],ymm3[13],ymm2[14],ymm3[14],ymm2[15],ymm3[15],ymm2[24],ymm3[24],ymm2[25],ymm3[25],ymm2[26],ymm3[26],ymm2[27],ymm3[27],ymm2[28],ymm3[28],ymm2[29],ymm3[29],ymm2[30],ymm3[30],ymm2[31],ymm3[31]
; AVX2-NEXT:    vpunpckhwd {{.*#+}} ymm3 = ymm4[4],ymm1[4],ymm4[5],ymm1[5],ymm4[6],ymm1[6],ymm4[7],ymm1[7],ymm4[12],ymm1[12],ymm4[13],ymm1[13],ymm4[14],ymm1[14],ymm4[15],ymm1[15]
; AVX2-NEXT:    vpunpckhwd {{.*#+}} ymm5 = ymm0[4],ymm2[4],ymm0[5],ymm2[5],ymm0[6],ymm2[6],ymm0[7],ymm2[7],ymm0[12],ymm2[12],ymm0[13],ymm2[13],ymm0[14],ymm2[14],ymm0[15],ymm2[15]
; AVX2-NEXT:    vpunpcklwd {{.*#+}} ymm1 = ymm4[0],ymm1[0],ymm4[1],ymm1[1],ymm4[2],ymm1[2],ymm4[3],ymm1[3],ymm4[8],ymm1[8],ymm4[9],ymm1[9],ymm4[10],ymm1[10],ymm4[11],ymm1[11]
; AVX2-NEXT:    vpunpcklwd {{.*#+}} ymm0 = ymm0[0],ymm2[0],ymm0[1],ymm2[1],ymm0[2],ymm2[2],ymm0[3],ymm2[3],ymm0[8],ymm2[8],ymm0[9],ymm2[9],ymm0[10],ymm2[10],ymm0[11],ymm2[11]
; AVX2-NEXT:    vinserti128 $1, %xmm3, %ymm1, %ymm2
; AVX2-NEXT:    vinserti128 $1, %xmm5, %ymm0, %ymm4
; AVX2-NEXT:    vperm2i128 {{.*#+}} ymm1 = ymm1[2,3],ymm3[2,3]
; AVX2-NEXT:    vperm2i128 {{.*#+}} ymm0 = ymm0[2,3],ymm5[2,3]
; AVX2-NEXT:    vmovdqa %ymm2, (%rdi)
; AVX2-NEXT:    vmovdqa %ymm4, 32(%rdi)
; AVX2-NEXT:    vmovdqa %ymm1, 64(%rdi)
; AVX2-NEXT:    vmovdqa %ymm0, 96(%rdi)
; AVX2-NEXT:    vzeroupper
; AVX2-NEXT:    retq
  %v1 = shufflevector <32 x i8> %x1, <32 x i8> %x2, <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>
  %v2 = shufflevector <32 x i8> %x3, <32 x i8> %x4, <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>
  %interleaved.vec = shufflevector <64 x i8> %v1, <64 x i8> %v2, <128 x i32> <i32 0, i32 32, i32 64, i32 96, i32 1, i32 33, i32 65, i32 97, i32 2, i32 34, i32 66, i32 98, i32 3, i32 35, i32 67, i32 99, i32 4, i32 36, i32 68, i32 100, i32 5, i32 37, i32 69, i32 101, i32 6, i32 38, i32 70, i32 102, i32 7, i32 39, i32 71, i32 103, i32 8, i32 40, i32 72, i32 104, i32 9, i32 41, i32 73, i32 105, i32 10, i32 42, i32 74, i32 106, i32 11, i32 43, i32 75, i32 107, i32 12, i32 44, i32 76, i32 108, i32 13, i32 45, i32 77, i32 109, i32 14, i32 46, i32 78, i32 110, i32 15, i32 47, i32 79, i32 111, i32 16, i32 48, i32 80, i32 112, i32 17, i32 49, i32 81, i32 113, i32 18, i32 50, i32 82, i32 114, i32 19, i32 51, i32 83, i32 115, i32 20, i32 52, i32 84, i32 116, i32 21, i32 53, i32 85, i32 117, i32 22, i32 54, i32 86, i32 118, i32 23, i32 55, i32 87, i32 119, i32 24, i32 56, i32 88, i32 120, i32 25, i32 57, i32 89, i32 121, i32 26, i32 58, i32 90, i32 122, i32 27, i32 59, i32 91, i32 123, i32 28, i32 60, i32 92, i32 124, i32 29, i32 61, i32 93, i32 125, i32 30, i32 62, i32 94, i32 126, i32 31, i32 63, i32 95, i32 127>
  store <128 x i8> %interleaved.vec, <128 x i8>* %p
ret void
}

