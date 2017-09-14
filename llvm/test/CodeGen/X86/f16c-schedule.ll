; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -print-schedule -mcpu=x86-64 -mattr=+f16c | FileCheck %s --check-prefix=CHECK --check-prefix=GENERIC
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -print-schedule -mcpu=ivybridge | FileCheck %s --check-prefix=CHECK --check-prefix=IVY
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -print-schedule -mcpu=haswell | FileCheck %s --check-prefix=CHECK --check-prefix=HASWELL
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -print-schedule -mcpu=skylake | FileCheck %s --check-prefix=CHECK --check-prefix=SKYLAKE
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -print-schedule -mcpu=btver2 | FileCheck %s --check-prefix=CHECK --check-prefix=BTVER2
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -print-schedule -mcpu=znver1 | FileCheck %s --check-prefix=CHECK --check-prefix=ZNVER1

define <4 x float> @test_vcvtph2ps_128(<8 x i16> %a0, <8 x i16> *%a1) {
; GENERIC-LABEL: test_vcvtph2ps_128:
; GENERIC:       # BB#0:
; GENERIC-NEXT:    vcvtph2ps (%rdi), %xmm1 # sched: [7:1.00]
; GENERIC-NEXT:    vcvtph2ps %xmm0, %xmm0 # sched: [3:1.00]
; GENERIC-NEXT:    vaddps %xmm0, %xmm1, %xmm0 # sched: [3:1.00]
; GENERIC-NEXT:    retq # sched: [1:1.00]
;
; IVY-LABEL: test_vcvtph2ps_128:
; IVY:       # BB#0:
; IVY-NEXT:    vcvtph2ps (%rdi), %xmm1 # sched: [7:1.00]
; IVY-NEXT:    vcvtph2ps %xmm0, %xmm0 # sched: [3:1.00]
; IVY-NEXT:    vaddps %xmm0, %xmm1, %xmm0 # sched: [3:1.00]
; IVY-NEXT:    retq # sched: [1:1.00]
;
; HASWELL-LABEL: test_vcvtph2ps_128:
; HASWELL:       # BB#0:
; HASWELL-NEXT:    vcvtph2ps (%rdi), %xmm1 # sched: [1:1.00]
; HASWELL-NEXT:    vcvtph2ps %xmm0, %xmm0 # sched: [2:1.00]
; HASWELL-NEXT:    vaddps %xmm0, %xmm1, %xmm0 # sched: [3:1.00]
; HASWELL-NEXT:    retq # sched: [2:1.00]
;
; SKYLAKE-LABEL: test_vcvtph2ps_128:
; SKYLAKE:       # BB#0:
; SKYLAKE-NEXT:    vcvtph2ps (%rdi), %xmm1 # sched: [4:0.50]
; SKYLAKE-NEXT:    vcvtph2ps %xmm0, %xmm0 # sched: [5:1.00]
; SKYLAKE-NEXT:    vaddps %xmm0, %xmm1, %xmm0 # sched: [4:0.50]
; SKYLAKE-NEXT:    retq # sched: [2:1.00]
;
; BTVER2-LABEL: test_vcvtph2ps_128:
; BTVER2:       # BB#0:
; BTVER2-NEXT:    vcvtph2ps (%rdi), %xmm1 # sched: [8:1.00]
; BTVER2-NEXT:    vcvtph2ps %xmm0, %xmm0 # sched: [3:1.00]
; BTVER2-NEXT:    vaddps %xmm0, %xmm1, %xmm0 # sched: [3:1.00]
; BTVER2-NEXT:    retq # sched: [4:1.00]
;
; ZNVER1-LABEL: test_vcvtph2ps_128:
; ZNVER1:       # BB#0:
; ZNVER1-NEXT:    vcvtph2ps (%rdi), %xmm1 # sched: [100:?]
; ZNVER1-NEXT:    vcvtph2ps %xmm0, %xmm0 # sched: [100:?]
; ZNVER1-NEXT:    vaddps %xmm0, %xmm1, %xmm0 # sched: [3:1.00]
; ZNVER1-NEXT:    retq # sched: [1:0.50]
  %1 = load <8 x i16>, <8 x i16> *%a1
  %2 = call <4 x float> @llvm.x86.vcvtph2ps.128(<8 x i16> %1)
  %3 = call <4 x float> @llvm.x86.vcvtph2ps.128(<8 x i16> %a0)
  %4 = fadd <4 x float> %2, %3
  ret <4 x float> %4
}
declare <4 x float> @llvm.x86.vcvtph2ps.128(<8 x i16>)

define <8 x float> @test_vcvtph2ps_256(<8 x i16> %a0, <8 x i16> *%a1) {
; GENERIC-LABEL: test_vcvtph2ps_256:
; GENERIC:       # BB#0:
; GENERIC-NEXT:    vcvtph2ps (%rdi), %ymm1 # sched: [7:1.00]
; GENERIC-NEXT:    vcvtph2ps %xmm0, %ymm0 # sched: [3:1.00]
; GENERIC-NEXT:    vaddps %ymm0, %ymm1, %ymm0 # sched: [3:1.00]
; GENERIC-NEXT:    retq # sched: [1:1.00]
;
; IVY-LABEL: test_vcvtph2ps_256:
; IVY:       # BB#0:
; IVY-NEXT:    vcvtph2ps (%rdi), %ymm1 # sched: [7:1.00]
; IVY-NEXT:    vcvtph2ps %xmm0, %ymm0 # sched: [3:1.00]
; IVY-NEXT:    vaddps %ymm0, %ymm1, %ymm0 # sched: [3:1.00]
; IVY-NEXT:    retq # sched: [1:1.00]
;
; HASWELL-LABEL: test_vcvtph2ps_256:
; HASWELL:       # BB#0:
; HASWELL-NEXT:    vcvtph2ps (%rdi), %ymm1 # sched: [1:1.00]
; HASWELL-NEXT:    vcvtph2ps %xmm0, %ymm0 # sched: [2:1.00]
; HASWELL-NEXT:    vaddps %ymm0, %ymm1, %ymm0 # sched: [3:1.00]
; HASWELL-NEXT:    retq # sched: [2:1.00]
;
; SKYLAKE-LABEL: test_vcvtph2ps_256:
; SKYLAKE:       # BB#0:
; SKYLAKE-NEXT:    vcvtph2ps (%rdi), %ymm1 # sched: [4:0.50]
; SKYLAKE-NEXT:    vcvtph2ps %xmm0, %ymm0 # sched: [7:1.00]
; SKYLAKE-NEXT:    vaddps %ymm0, %ymm1, %ymm0 # sched: [4:0.50]
; SKYLAKE-NEXT:    retq # sched: [2:1.00]
;
; BTVER2-LABEL: test_vcvtph2ps_256:
; BTVER2:       # BB#0:
; BTVER2-NEXT:    vcvtph2ps (%rdi), %ymm1 # sched: [8:1.00]
; BTVER2-NEXT:    vcvtph2ps %xmm0, %ymm0 # sched: [3:1.00]
; BTVER2-NEXT:    vaddps %ymm0, %ymm1, %ymm0 # sched: [3:2.00]
; BTVER2-NEXT:    retq # sched: [4:1.00]
;
; ZNVER1-LABEL: test_vcvtph2ps_256:
; ZNVER1:       # BB#0:
; ZNVER1-NEXT:    vcvtph2ps (%rdi), %ymm1 # sched: [100:?]
; ZNVER1-NEXT:    vcvtph2ps %xmm0, %ymm0 # sched: [100:?]
; ZNVER1-NEXT:    vaddps %ymm0, %ymm1, %ymm0 # sched: [3:1.00]
; ZNVER1-NEXT:    retq # sched: [1:0.50]
  %1 = load <8 x i16>, <8 x i16> *%a1
  %2 = call <8 x float> @llvm.x86.vcvtph2ps.256(<8 x i16> %1)
  %3 = call <8 x float> @llvm.x86.vcvtph2ps.256(<8 x i16> %a0)
  %4 = fadd <8 x float> %2, %3
  ret <8 x float> %4
}
declare <8 x float> @llvm.x86.vcvtph2ps.256(<8 x i16>)

define <8 x i16> @test_vcvtps2ph_128(<4 x float> %a0, <4 x float> %a1, <4 x i16> *%a2) {
; GENERIC-LABEL: test_vcvtps2ph_128:
; GENERIC:       # BB#0:
; GENERIC-NEXT:    vcvtps2ph $0, %xmm0, %xmm0 # sched: [3:1.00]
; GENERIC-NEXT:    vcvtps2ph $0, %xmm1, (%rdi) # sched: [7:1.00]
; GENERIC-NEXT:    retq # sched: [1:1.00]
;
; IVY-LABEL: test_vcvtps2ph_128:
; IVY:       # BB#0:
; IVY-NEXT:    vcvtps2ph $0, %xmm0, %xmm0 # sched: [3:1.00]
; IVY-NEXT:    vcvtps2ph $0, %xmm1, (%rdi) # sched: [7:1.00]
; IVY-NEXT:    retq # sched: [1:1.00]
;
; HASWELL-LABEL: test_vcvtps2ph_128:
; HASWELL:       # BB#0:
; HASWELL-NEXT:    vcvtps2ph $0, %xmm0, %xmm0 # sched: [4:1.00]
; HASWELL-NEXT:    vcvtps2ph $0, %xmm1, (%rdi) # sched: [4:1.00]
; HASWELL-NEXT:    retq # sched: [2:1.00]
;
; SKYLAKE-LABEL: test_vcvtps2ph_128:
; SKYLAKE:       # BB#0:
; SKYLAKE-NEXT:    vcvtps2ph $0, %xmm0, %xmm0 # sched: [5:1.00]
; SKYLAKE-NEXT:    vcvtps2ph $0, %xmm1, (%rdi) # sched: [5:1.00]
; SKYLAKE-NEXT:    retq # sched: [2:1.00]
;
; BTVER2-LABEL: test_vcvtps2ph_128:
; BTVER2:       # BB#0:
; BTVER2-NEXT:    vcvtps2ph $0, %xmm0, %xmm0 # sched: [3:1.00]
; BTVER2-NEXT:    vcvtps2ph $0, %xmm1, (%rdi) # sched: [8:1.00]
; BTVER2-NEXT:    retq # sched: [4:1.00]
;
; ZNVER1-LABEL: test_vcvtps2ph_128:
; ZNVER1:       # BB#0:
; ZNVER1-NEXT:    vcvtps2ph $0, %xmm0, %xmm0 # sched: [100:?]
; ZNVER1-NEXT:    vcvtps2ph $0, %xmm1, (%rdi) # sched: [100:?]
; ZNVER1-NEXT:    retq # sched: [1:0.50]
  %1 = call <8 x i16> @llvm.x86.vcvtps2ph.128(<4 x float> %a0, i32 0)
  %2 = call <8 x i16> @llvm.x86.vcvtps2ph.128(<4 x float> %a1, i32 0)
  %3 = shufflevector <8 x i16> %2, <8 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  store <4 x i16> %3, <4 x i16> *%a2
  ret <8 x i16> %1
}
declare <8 x i16> @llvm.x86.vcvtps2ph.128(<4 x float>, i32)

define <8 x i16> @test_vcvtps2ph_256(<8 x float> %a0, <8 x float> %a1, <8 x i16> *%a2) {
; GENERIC-LABEL: test_vcvtps2ph_256:
; GENERIC:       # BB#0:
; GENERIC-NEXT:    vcvtps2ph $0, %ymm0, %xmm0 # sched: [3:1.00]
; GENERIC-NEXT:    vcvtps2ph $0, %ymm1, (%rdi) # sched: [7:1.00]
; GENERIC-NEXT:    vzeroupper
; GENERIC-NEXT:    retq # sched: [1:1.00]
;
; IVY-LABEL: test_vcvtps2ph_256:
; IVY:       # BB#0:
; IVY-NEXT:    vcvtps2ph $0, %ymm0, %xmm0 # sched: [3:1.00]
; IVY-NEXT:    vcvtps2ph $0, %ymm1, (%rdi) # sched: [7:1.00]
; IVY-NEXT:    vzeroupper
; IVY-NEXT:    retq # sched: [1:1.00]
;
; HASWELL-LABEL: test_vcvtps2ph_256:
; HASWELL:       # BB#0:
; HASWELL-NEXT:    vcvtps2ph $0, %ymm0, %xmm0 # sched: [6:1.00]
; HASWELL-NEXT:    vcvtps2ph $0, %ymm1, (%rdi) # sched: [6:1.00]
; HASWELL-NEXT:    vzeroupper # sched: [4:1.00]
; HASWELL-NEXT:    retq # sched: [2:1.00]
;
; SKYLAKE-LABEL: test_vcvtps2ph_256:
; SKYLAKE:       # BB#0:
; SKYLAKE-NEXT:    vcvtps2ph $0, %ymm0, %xmm0 # sched: [7:1.00]
; SKYLAKE-NEXT:    vcvtps2ph $0, %ymm1, (%rdi) # sched: [7:1.00]
; SKYLAKE-NEXT:    vzeroupper # sched: [4:1.00]
; SKYLAKE-NEXT:    retq # sched: [2:1.00]
;
; BTVER2-LABEL: test_vcvtps2ph_256:
; BTVER2:       # BB#0:
; BTVER2-NEXT:    vcvtps2ph $0, %ymm0, %xmm0 # sched: [3:1.00]
; BTVER2-NEXT:    vcvtps2ph $0, %ymm1, (%rdi) # sched: [8:1.00]
; BTVER2-NEXT:    retq # sched: [4:1.00]
;
; ZNVER1-LABEL: test_vcvtps2ph_256:
; ZNVER1:       # BB#0:
; ZNVER1-NEXT:    vcvtps2ph $0, %ymm0, %xmm0 # sched: [100:?]
; ZNVER1-NEXT:    vcvtps2ph $0, %ymm1, (%rdi) # sched: [100:?]
; ZNVER1-NEXT:    vzeroupper # sched: [100:?]
; ZNVER1-NEXT:    retq # sched: [1:0.50]
  %1 = call <8 x i16> @llvm.x86.vcvtps2ph.256(<8 x float> %a0, i32 0)
  %2 = call <8 x i16> @llvm.x86.vcvtps2ph.256(<8 x float> %a1, i32 0)
  store <8 x i16> %2, <8 x i16> *%a2
  ret <8 x i16> %1
}
declare <8 x i16> @llvm.x86.vcvtps2ph.256(<8 x float>, i32)
