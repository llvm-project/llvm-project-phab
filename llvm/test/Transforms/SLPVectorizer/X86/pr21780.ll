; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -slp-vectorizer -mtriple=x86_64-unknown-linux-gnu -mcpu=bdver2 -S | FileCheck %s

define <4 x double> @foo(double* %ptr) #0 {
; CHECK-LABEL: @foo(
; CHECK-NEXT:    [[ARRAYIDX2:%.*]] = getelementptr inbounds double, double* [[PTR:%.*]], i64 2
; CHECK-NEXT:    [[TMP1:%.*]] = getelementptr double, double* [[PTR]], i64 3
; CHECK-NEXT:    [[TMP2:%.*]] = getelementptr double, double* [[PTR]], i64 1
; CHECK-NEXT:    [[TMP3:%.*]] = bitcast double* [[PTR]] to <4 x double>*
; CHECK-NEXT:    [[TMP4:%.*]] = load <4 x double>, <4 x double>* [[TMP3]], align 8
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <4 x double> [[TMP4]], i32 0
; CHECK-NEXT:    [[INS0:%.*]] = insertelement <4 x double> undef, double [[TMP5]], i32 0
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <4 x double> [[TMP4]], i32 1
; CHECK-NEXT:    [[TMP7:%.*]] = insertelement <4 x double> [[INS0]], double [[TMP6]], i32 1
; CHECK-NEXT:    [[TMP8:%.*]] = extractelement <4 x double> [[TMP4]], i32 2
; CHECK-NEXT:    [[INS2:%.*]] = insertelement <4 x double> [[TMP7]], double [[TMP8]], i32 2
; CHECK-NEXT:    [[TMP9:%.*]] = extractelement <4 x double> [[TMP4]], i32 3
; CHECK-NEXT:    [[TMP10:%.*]] = insertelement <4 x double> [[INS2]], double [[TMP9]], i32 3
; CHECK-NEXT:    [[SHUFFLE:%.*]] = shufflevector <4 x double> [[TMP10]], <4 x double> undef, <4 x i32> <i32 0, i32 0, i32 2, i32 2>
; CHECK-NEXT:    ret <4 x double> [[SHUFFLE]]
;
  %arrayidx2 = getelementptr inbounds double, double* %ptr, i64 2
  %ld0 = load double, double* %ptr, align 8, !speculation.marker !0
  %ld2 = load double, double* %arrayidx2, align 8, !speculation.marker !1
  %ins0 = insertelement <4 x double> undef, double %ld0, i32 0
  %ins2 = insertelement <4 x double> %ins0, double %ld2, i32 2
  %shuffle = shufflevector <4 x double> %ins2, <4 x double> undef, <4 x i32> <i32 0, i32 0, i32 2, i32 2>
  ret <4 x double> %shuffle
}

define <4 x double> @bar(double* %ptr) #0 {
; CHECK-LABEL: @bar(
; CHECK-NEXT:    [[ARRAYIDX1:%.*]] = getelementptr inbounds double, double* [[PTR:%.*]], i64 1
; CHECK-NEXT:    [[ARRAYIDX2:%.*]] = getelementptr inbounds double, double* [[PTR]], i64 2
; CHECK-NEXT:    [[TMP1:%.*]] = getelementptr double, double* [[PTR]], i64 3
; CHECK-NEXT:    [[TMP2:%.*]] = bitcast double* [[PTR]] to <4 x double>*
; CHECK-NEXT:    [[TMP3:%.*]] = load <4 x double>, <4 x double>* [[TMP2]], align 8
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <4 x double> [[TMP3]], i32 0
; CHECK-NEXT:    [[TMP5:%.*]] = insertelement <4 x double> undef, double [[TMP4]], i32 0
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <4 x double> [[TMP3]], i32 1
; CHECK-NEXT:    [[INS1:%.*]] = insertelement <4 x double> [[TMP5]], double [[TMP6]], i32 1
; CHECK-NEXT:    [[TMP7:%.*]] = extractelement <4 x double> [[TMP3]], i32 2
; CHECK-NEXT:    [[INS2:%.*]] = insertelement <4 x double> [[INS1]], double [[TMP7]], i32 2
; CHECK-NEXT:    [[TMP8:%.*]] = extractelement <4 x double> [[TMP3]], i32 3
; CHECK-NEXT:    [[TMP9:%.*]] = insertelement <4 x double> [[INS2]], double [[TMP8]], i32 3
; CHECK-NEXT:    [[SHUFFLE:%.*]] = shufflevector <4 x double> [[TMP9]], <4 x double> undef, <4 x i32> <i32 1, i32 1, i32 2, i32 2>
; CHECK-NEXT:    ret <4 x double> [[SHUFFLE]]
;
  %arrayidx1 = getelementptr inbounds double, double* %ptr, i64 1
  %arrayidx2 = getelementptr inbounds double, double* %ptr, i64 2
  %ld1 = load double, double* %arrayidx1, align 8, !speculation.marker !2
  %ld2 = load double, double* %arrayidx2, align 8, !speculation.marker !3
  %ins1 = insertelement <4 x double> undef, double %ld1, i32 1
  %ins2 = insertelement <4 x double> %ins1, double %ld2, i32 2
  %shuffle = shufflevector <4 x double> %ins2, <4 x double> undef, <4 x i32> <i32 1, i32 1, i32 2, i32 2>
  ret <4 x double> %shuffle
}

attributes #0 = { "target-cpu"="bdver2" }

!0 = !{i64 1, i64 3}
!1 = !{i64 -1, i64 1}
!2 = !{i64 -1, i64 2}
!3 = !{i64 -2, i64 1}
