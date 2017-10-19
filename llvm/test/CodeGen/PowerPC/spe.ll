; RUN: llc -verify-machineinstrs < %s -mtriple=powerpc-unknown-linux-gnu \
; RUN:          -mattr=+spe |  FileCheck %s

declare float @llvm.fabs.float(float)
define float @test_float_abs(float %a) #0 {
; CHECK-LABEL: test_float_abs
  entry:
    %0 = tail call float @llvm.fabs.float(float %a)
    ret float %0
}
; CHECK: efsabs 3, 3
; CHECK: blr

define float @test_fnabs(float %a) #0 {
  entry:
    %0 = tail call float @llvm.fabs.float(float %a)
    %sub = fsub float -0.000000e+00, %0
    ret float %sub
}
; CHECK-LABEL: @test_fnabs
; CHECK-NO: efsnabs
; CHECK: blr

define float @test_fdiv(float %a, float %b) {
entry:
  %v = fdiv float %a, %b
  ret float %v

; CHECK-LABEL: test_fdiv
; CHECK: efsdiv 
; CHECK: blr
}

define float @test_fmul(float %a, float %b) {
  entry:
  %v = fmul float %a, %b
  ret float %v
; CHECK-LABEL @test_fmul
; CHECK: efsmul
; CHECK: blr
}

define float @test_fadd(float %a, float %b) {
  entry:
  %v = fadd float %a, %b
  ret float %v
; CHECK-LABEL @test_fadd
; CHECK: efsadd
; CHECK: blr
}

define float @test_fsub(float %a, float %b) {
  entry:
  %v = fsub float %a, %b
  ret float %v
; CHECK-LABEL @test_fsub
; CHECK: efssub
; CHECK: blr
}

define float @test_fneg(float %a) {
  entry:
  %v = fsub float -0.0, %a
  ret float %v

; CHECK-LABEL @test_fneg
; CHECK: efsneg
; CHECK: blr
}

define float @test_dtos(double %a) {
  entry:
  %v = fptrunc double %a to float
  ret float %v
; CHECK-LABEL: test_dtos
; CHECK: efscfd
; CHECK: blr
}

define i1 @test_fcmpgt(float %a, float %b) {
  entry:
  %r = fcmp ogt float %a, %b
  ret i1 %r
}

define i1 @test_fcmpeq(float %a, float %b) {
  entry:
  %r = fcmp oeq float %a, %b
  ret i1 %r
}

define i1 @test_fcmplt(float %a, float %b) {
  entry:
  %r = fcmp olt float %a, %b
  ret i1 %r
}

define i32 @test_ftoui(float %a) {
  %v = fptoui float %a to i32
  ret i32 %v
}

define i32 @test_ftosi(float %a) {
  %v = fptosi float %a to i32
  ret i32 %v
}

define float @test_ffromui(i32 %a) {
  %v = uitofp i32 %a to float
  ret float %v
}

define float @test_ffromsi(i32 %a) {
  %v = sitofp i32 %a to float
  ret float %v
}

; Double tests

define double @test_double_abs(double %aa) #0 {

; CHECK-LABEL: test_double_abs

  entry:
    %0 = tail call double @llvm.fabs.f64(double %aa) #2
    ret double %0
}
; CHECK: blr

; Function Attrs: nounwind readnone
declare double @llvm.fabs.f64(double) #1

define double @test_dnabs(double %aa) #0 {
  entry:
    %0 = tail call double @llvm.fabs.f64(double %aa) #2
    %sub = fsub double -0.000000e+00, %0
    ;%add2 = fadd double %aa, %sub
    ;ret double %add2
    %add = fadd double %sub, 1.0
    ret double %add
}
; CHECK-LABEL: @test_dnabs
; CHECK-NO: efdnabs
; CHECK: blr

define double @test_ddiv(double %a, double %b) {
entry:
  %v = fdiv double %a, %b
  ret double %v

; CHECK-LABEL: test_ddiv
; CHECK: efddiv 
; CHECK: blr
}

define double @test_dmul(double %a, double %b) {
  entry:
  %v = fmul double %a, %b
  ret double %v
; CHECK-LABEL @test_dmul
; CHECK: efdmul
; CHECK: blr
}

define double @test_dadd(double %a, double %b) {
  entry:
  %v = fadd double %a, %b
  ret double %v
; CHECK-LABEL @test_dadd
; CHECK: efdadd
; CHECK: blr
}

define double @test_dsub(double %a, double %b) {
  entry:
  %v = fsub double %a, %b
  ret double %v
; CHECK-LABEL @test_dsub
; CHECK: efdsub
; CHECK: blr
}

define double @test_dneg(double %a) {
  entry:
  %v = fsub double -0.0, %a
  ret double %v

; CHECK-LABEL @test_dneg
; CHECK: blr
}

define double @test_stod(float %a) {
  entry:
  %v = fpext float %a to double
  ret double %v
; CHECK-LABEL: test_stod
; CHECK: efdcfs
; CHECK: blr
}

define i1 @test_dcmpgt(double %a, double %b) {
  entry:
  %r = fcmp ogt double %a, %b
  ret i1 %r
}

define i1 @test_dcmpeq(double %a, double %b) {
  entry:
  %r = fcmp oeq double %a, %b
  ret i1 %r
}

define i1 @test_dcmplt(double %a, double %b) {
  entry:
  %r = fcmp olt double %a, %b
  ret i1 %r
}

define i32 @test_dtoui(double %a) {
  %v = fptoui double %a to i32
  ret i32 %v
}

define i32 @test_dtosi(double %a) {
  %v = fptosi double %a to i32
  ret i32 %v
}

define double @test_dfromui(i32 %a) {
  %v = uitofp i32 %a to double
  ret double %v
}

define double @test_dfromsi(i32 %a) {
  %v = sitofp i32 %a to double
  ret double %v
}

; Vector float tests

define <2 x float> @test_float_abs_v(<2 x float> %aa) #0 {

; CHECK-LABEL: test_float_abs_v

  entry:
    %0 = tail call <2 x float> @llvm.fabs.v2f32(<2 x float> %aa) #2
    ret <2 x float> %0
}
; Function Attrs: nounwind readnone
declare <2 x float> @llvm.fabs.v2f32(<2 x float>) #1

; CHECK: evfsabs 3, 3
; CHECK: blr

define <2 x float> @test2_float_abs_v(<2 x float> %aa) #0 {

; CHECK-LABEL: test2_float_abs_v

  entry:
    %0 = tail call <2 x float> @llvm.fabs.v2f32(<2 x float> %aa) #2
    %sub = fsub <2 x float> <float -0.000000e+00, float -0.000000e+00>, %0
    ret <2 x float> %sub
}

; CHECK: evfsnabs 3, 3
; CHECK: blr

define <2 x float> @test_fneg_v(<2 x float> %a) {
  entry:
  %v = fsub <2 x float> <float-0.0, float -0.0>, %a
  ret <2 x float> %v

; CHECK-LABEL @test_fneg_v
; CHECK: evfsneg
; CHECK: blr
}

define <2 x float> @test_fdiv_v(<2 x float> %a, <2 x float> %b) {
entry:
  %v = fdiv <2 x float> %a, %b
  ret <2 x float> %v

; CHECK-LABEL: test_fdiv_v
; CHECK: evfsdiv 
; CHECK: blr
}

define <2 x float> @test_fmul_v(<2 x float> %a, <2 x float> %b) {
  entry:
  %v = fmul <2 x float> %a, %b
  ret <2 x float> %v
; CHECK-LABEL @test_fmul_v
; CHECK: evfsmul
; CHECK: blr
}

define <2 x float> @test_fadd_v(<2 x float> %a, <2 x float> %b) {
  entry:
  %v = fadd <2 x float> %a, %b
  ret <2 x float> %v
; CHECK-LABEL @test_fadd_v
; CHECK: evfsadd
; CHECK: blr
}

define <2 x float> @test_fsub_v(<2 x float> %a, <2 x float> %b) {
  entry:
  %v = fsub <2 x float> %a, %b
  ret <2 x float> %v
; CHECK-LABEL @test_fsub_v
; CHECK: evfssub
; CHECK: blr
}

;define <2 x i1> @test_fcmpgt_v(<2 x float> %a, <2 x float> %b) {
;  entry:
;  %r = fcmp ogt <2 x float> %a, %b
;  ret <2 x i1> %r
;}
;
;define <2 x i1> @test_fcmpeq_v(<2 x float> %a, <2 x float> %b) {
;  entry:
;  %r = fcmp oeq <2 x float> %a, %b
;  ret <2 x i1> %r
;}
;
;define <2 x i1> @test_fcmplt_v(<2 x float> %a, <2 x float> %b) {
;  entry:
;  %r = fcmp olt <2 x float> %a, %b
;  ret <2 x i1> %r
;}
;
define <2 x i32> @test_ftoui_v(<2 x float> %a) {
  %v = fptoui <2 x float> %a to <2 x i32>
  ret <2 x i32> %v
}

define <2 x i32> @test_ftosi_v(<2 x float> %a) {
  %v = fptosi <2 x float> %a to <2 x i32>
  ret <2 x i32> %v
}

define <2 x float> @test_ffromui_v(<2 x i32> %a) {
  %v = uitofp <2 x i32> %a to <2 x float>
  ret <2 x float> %v
}

define <2 x float> @test_ffromsi_v(<2 x i32> %a) {
  %v = sitofp <2 x i32> %a to <2 x float>
  ret <2 x float> %v
}

; Vector int tests

define <2 x i32> @test_i32_abs_v(<2 x i32> %aa) #0 {

; CHECK-LABEL: test_i32_abs_v

  entry:
    %0 = tail call <2 x i32> @llvm.ppc.spe.evabs(<2 x i32> %aa) #2
    ret <2 x i32> %0
}

declare <2 x i32> @llvm.ppc.spe.evabs(<2 x i32>) #1

; CHECK: evabs 3, 3
; CHECK: blr

define <2 x i32> @test_neg_v(<2 x i32> %a) {
  entry:
  %v = sub <2 x i32> zeroinitializer, %a
  ret <2 x i32> %v

; CHECK-LABEL @test_neg_v
; CHECK: evneg
; CHECK: blr
}

define <2 x i32> @test_nor_v(<2 x i32> %a, <2 x i32> %b) {
  entry:
  %v = or <2 x i32> %a, %b
  %r = xor <2 x i32> %v, <i32 -1, i32 -1>
  ret <2 x i32> %r

; CHECK-LABEL @test_nor_v
; CHECK: evnor
; CHECK: blr
}

define <2 x i32> @test_orc_v(<2 x i32> %a, <2 x i32> %b) {
  entry:
  %v = xor <2 x i32> %b, <i32 -1, i32 -1>
  %r = or <2 x i32> %a, %v
  ret <2 x i32> %r

; CHECK-LABEL @test_orc_v
; CHECK: evorc
; CHECK: blr
}

define <2 x i32> @test_nand_v(<2 x i32> %a, <2 x i32> %b) {
  entry:
  %v = and <2 x i32> %a, %b
  %r = xor <2 x i32> %v, <i32 -1, i32 -1>
  ret <2 x i32> %r

; CHECK-LABEL @test_nand_v
; CHECK: evnand
; CHECK: blr
}

define <2 x i32> @test_andc_v(<2 x i32> %a, <2 x i32> %b) {
  entry:
  %v = xor <2 x i32> %b, <i32 -1, i32 -1>
  %r = and <2 x i32> %a, %v
  ret <2 x i32> %r

; CHECK-LABEL @test_andc_v
; CHECK: evandc
; CHECK: blr
}

define <2 x i32> @test_xor_v(<2 x i32> %a, <2 x i32> %b) {
  entry:
  %v = xor <2 x i32> %a, %b
  ret <2 x i32> %v

; CHECK-LABEL @test_xor_v
; CHECK: evxor
; CHECK: blr
}

define <2 x i32> @test_slw_v(<2 x i32> %a, <2 x i32> %b) {
  entry:
  %v = shl <2 x i32> %a, %b
  ret <2 x i32> %v
}

define <2 x i32> @test_srws_v(<2 x i32> %a, <2 x i32> %b) {
  entry:
  %v = ashr <2 x i32> %a, %b
  ret <2 x i32> %v
}

define <2 x i32> @test_srwu_v(<2 x i32> %a, <2 x i32> %b) {
  entry:
  %v = lshr <2 x i32> %a, %b
  ret <2 x i32> %v
}

define <2 x i32> @test_divs_v(<2 x i32> %a, <2 x i32> %b) {
entry:
  %v = sdiv <2 x i32> %a, %b
  ret <2 x i32> %v

; CHECK-LABEL: test_divs_v
; CHECK: evdivws
; CHECK: blr
}

define <2 x i32> @test_divu_v(<2 x i32> %a, <2 x i32> %b) {
entry:
  %v = udiv <2 x i32> %a, %b
  ret <2 x i32> %v

; CHECK-LABEL: test_divu_v
; CHECK: evdivwu
; CHECK: blr
}

define <2 x i32> @test_mul_v(<2 x i32> %a, <2 x i32> %b) {
  entry:
  %v = mul <2 x i32> %a, %b
  ret <2 x i32> %v
; CHECK-LABEL @test_mul_v
; CHECK: evmwlumi
; CHECK: blr
}

define <2 x i32> @test_add_v(<2 x i32> %a, <2 x i32> %b) {
  entry:
  %v = add <2 x i32> %a, %b
  ret <2 x i32> %v
; CHECK-LABEL @test_add_v
; CHECK: evadd
; CHECK: blr
}

define <2 x i32> @test_sub_v(<2 x i32> %a, <2 x i32> %b) {
  entry:
  %v = sub <2 x i32> %a, %b
  ret <2 x i32> %v
; CHECK-LABEL @test_sub_v
; CHECK: evsubf
; CHECK: blr
}

;define <2 x i1> @test_cmpgt_v(<2 x i32> %a, <2 x i32> %b) {
;  entry:
;  %r = cmp ogt <2 x i32> %a, %b
;  ret <2 x i1> %r
;}
;
;define <2 x i1> @test_cmpeq_v(<2 x i32> %a, <2 x i32> %b) {
;  entry:
;  %r = cmp oeq <2 x i32> %a, %b
;  ret <2 x i1> %r
;}
;
;define <2 x i1> @test_cmplt_v(<2 x i32> %a, <2 x i32> %b) {
;  entry:
;  %r = cmp olt <2 x i32> %a, %b
;  ret <2 x i1> %r
;}
;
