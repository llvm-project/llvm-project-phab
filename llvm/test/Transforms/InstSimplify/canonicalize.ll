; RUN: opt -instsimplify -S < %s | FileCheck %s

declare float @llvm.canonicalize.f32(float)
declare <2 x float> @llvm.canonicalize.v2f32(<2 x float>)

; CHECK-LABEL: @fold_zero(
; CHECK-NEXT: ret float 0.000000e+00
define float @fold_zero() {
  %ret = call float @llvm.canonicalize.f32(float 0.0)
  ret float %ret
}

; CHECK-LABEL: @fold_negzero(
; CHECK-NEXT: ret float -0.000000e+00
define float @fold_negzero() {
  %ret = call float @llvm.canonicalize.f32(float -0.0)
  ret float %ret
}

; CHECK-LABEL: @fold_zero_vector(
; CHECK-NEXT: ret <2 x float> zeroinitializer
define <2 x float> @fold_zero_vector() {
  %ret = call <2 x float> @llvm.canonicalize.v2f32(<2 x float> zeroinitializer)
  ret <2 x float> %ret
}

; CHECK-LABEL: @fold_negzero_vector(
; CHECK-NEXT: ret <2 x float> <float -0.000000e+00, float -0.000000e+00>
define <2 x float> @fold_negzero_vector() {
  %ret = call <2 x float> @llvm.canonicalize.v2f32(<2 x float> <float -0.0, float -0.0>)
  ret <2 x float> %ret
}

; FIXME: Fold to -0.0 splat
; CHECK-LABEL: @fold_negzero_vector_partialundef(
; CHECK-NEXT: %ret = call <2 x float> @llvm.canonicalize.v2f32(<2 x float> <float -0.000000e+00, float undef>)
; CHECK-NEXT: ret <2 x float> %ret
define <2 x float> @fold_negzero_vector_partialundef() {
  %ret = call <2 x float> @llvm.canonicalize.v2f32(<2 x float> <float -0.0, float undef>)
  ret <2 x float> %ret
}

; CHECK-LABEL: @fold_undef(
; CHECK-NEXT: ret float 0.000000e+00
define float @fold_undef() {
  %ret = call float @llvm.canonicalize.f32(float undef)
  ret float %ret
}

; CHECK-LABEL: @fold_undef_vector(
; CHECK-NEXT: ret <2 x float> zeroinitializer
define <2 x float> @fold_undef_vector() {
  %ret = call <2 x float> @llvm.canonicalize.v2f32(<2 x float> undef)
  ret <2 x float> %ret
}

; CHECK-LABEL: @no_fold_denorm(
; CHECK-NEXT: %ret = call float @llvm.canonicalize.f32(float 0x380FFFFFC0000000)
; CHECK-NEXT: ret float %ret
define float @no_fold_denorm() {
  %ret = call float @llvm.canonicalize.f32(float bitcast (i32 8388607 to float))
  ret float %ret
}

; CHECK-LABEL: @no_fold_inf(
; CHECK-NEXT: %ret = call float @llvm.canonicalize.f32(float 0x7FF0000000000000)
; CHECK-NEXT: ret float %ret
define float @no_fold_inf() {
  %ret = call float @llvm.canonicalize.f32(float 0x7FF0000000000000)
  ret float %ret
}

; CHECK-LABEL: @no_fold_qnan(
; CHECK-NEXT: %ret = call float @llvm.canonicalize.f32(float 0x7FF8000000000000)
; CHECK-NEXT: ret float %ret
define float @no_fold_qnan() {
  %ret = call float @llvm.canonicalize.f32(float 0x7FF8000000000000)
  ret float %ret
}

; CHECK-LABEL: @no_fold_snan(
; CHECK-NEXT: %ret = call float @llvm.canonicalize.f32(float 0x7FF0000020000000)
; CHECK-NEXT: ret float %ret
define float @no_fold_snan() {
  %ret = call float @llvm.canonicalize.f32(float bitcast (i32 2139095041 to float))
  ret float %ret
}

; CHECK-LABEL: @fold_normal(
; CHECK-NEXT: ret float 4.000000e+00
define float @fold_normal() {
  %ret = call float @llvm.canonicalize.f32(float 4.0)
  ret float %ret
}
