; RUN: opt < %s -instcombine -S | FileCheck %s

define <4 x double> @foo(double* %ptr) {
; CHECK-LABEL: @foo(
; CHECK-NEXT:    [[ARRAYIDX2:%.*]] = getelementptr inbounds double, double* [[PTR:%.*]], i64 2
; CHECK-NEXT:    [[LD0:%.*]] = load double, double* [[PTR]], align 8, !speculation.marker !0
; CHECK-NEXT:    [[LD2:%.*]] = load double, double* [[ARRAYIDX2]], align 8, !speculation.marker !1
; CHECK-NEXT:    [[INS0:%.*]] = insertelement <4 x double> undef, double [[LD0]], i32 0
; CHECK-NEXT:    [[INS2:%.*]] = insertelement <4 x double> [[INS0]], double [[LD2]], i32 2
; CHECK-NEXT:    [[SHUFFLE:%.*]] = shufflevector <4 x double> [[INS2]], <4 x double> undef, <4 x i32> <i32 0, i32 0, i32 2, i32 2>
; CHECK-NEXT:    ret <4 x double> [[SHUFFLE]]
;
  %arrayidx0 = getelementptr inbounds double, double* %ptr, i64 0
  %arrayidx1 = getelementptr inbounds double, double* %ptr, i64 1
  %arrayidx2 = getelementptr inbounds double, double* %ptr, i64 2
  %arrayidx3 = getelementptr inbounds double, double* %ptr, i64 3

  %ld0 = load double, double* %arrayidx0
  %ld1 = load double, double* %arrayidx1
  %ld2 = load double, double* %arrayidx2
  %ld3 = load double, double* %arrayidx3

  %ins0 = insertelement <4 x double> undef, double %ld0, i32 0
  %ins1 = insertelement <4 x double> %ins0, double %ld1, i32 1
  %ins2 = insertelement <4 x double> %ins1, double %ld2, i32 2
  %ins3 = insertelement <4 x double> %ins2, double %ld3, i32 3

  %shuffle = shufflevector <4 x double> %ins3, <4 x double> undef, <4 x i32> <i32 0, i32 0, i32 2, i32 2>
  ret <4 x double> %shuffle
}

define <4 x double> @bar(double* %ptr) {
; CHECK-LABEL: @bar(
; CHECK-NEXT:    [[ARRAYIDX1:%.*]] = getelementptr inbounds double, double* [[PTR:%.*]], i64 1
; CHECK-NEXT:    [[ARRAYIDX2:%.*]] = getelementptr inbounds double, double* [[PTR]], i64 2
; CHECK-NEXT:    [[LD1:%.*]] = load double, double* [[ARRAYIDX1]], align 8, !speculation.marker !2
; CHECK-NEXT:    [[LD2:%.*]] = load double, double* [[ARRAYIDX2]], align 8, !speculation.marker !3
; CHECK-NEXT:    [[INS1:%.*]] = insertelement <4 x double> undef, double [[LD1]], i32 1
; CHECK-NEXT:    [[INS2:%.*]] = insertelement <4 x double> [[INS1]], double [[LD2]], i32 2
; CHECK-NEXT:    [[SHUFFLE:%.*]] = shufflevector <4 x double> [[INS2]], <4 x double> undef, <4 x i32> <i32 1, i32 1, i32 2, i32 2>
; CHECK-NEXT:    ret <4 x double> [[SHUFFLE]]
;
  %arrayidx0 = getelementptr inbounds double, double* %ptr, i64 0
  %arrayidx1 = getelementptr inbounds double, double* %ptr, i64 1
  %arrayidx2 = getelementptr inbounds double, double* %ptr, i64 2
  %arrayidx3 = getelementptr inbounds double, double* %ptr, i64 3

  %ld0 = load double, double* %arrayidx0
  %ld1 = load double, double* %arrayidx1
  %ld2 = load double, double* %arrayidx2
  %ld3 = load double, double* %arrayidx3

  %ins0 = insertelement <4 x double> undef, double %ld0, i32 0
  %ins1 = insertelement <4 x double> %ins0, double %ld1, i32 1
  %ins2 = insertelement <4 x double> %ins1, double %ld2, i32 2
  %ins3 = insertelement <4 x double> %ins2, double %ld3, i32 3

  %shuffle = shufflevector <4 x double> %ins3, <4 x double> undef, <4 x i32> <i32 1, i32 1, i32 2, i32 2>
  ret <4 x double> %shuffle
}

; CHECK: !0 = !{i64 1, i64 3}
; CHECK: !1 = !{i64 -1, i64 1}
; CHECK: !2 = !{i64 -1, i64 2}
; CHECK: !3 = !{i64 -2, i64 1}
