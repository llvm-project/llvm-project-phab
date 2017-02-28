; RUN: llc %s -o - | FileCheck %s


target triple = "x86_64-unknown-linux-gnu"

define void @merge_double(double* noalias nocapture %st, double* noalias nocapture readonly %ld) #0 {
  %ld_idx1 = getelementptr inbounds double, double* %ld, i64 1
  %ld0 = load double, double* %ld, align 8, !tbaa !2
  %ld1 = load double, double* %ld_idx1, align 8, !tbaa !2

  %st_idx1 = getelementptr inbounds double, double* %st, i64 1
  %st_idx2 = getelementptr inbounds double, double* %st, i64 2
  %st_idx3 = getelementptr inbounds double, double* %st, i64 3

  store double %ld0, double* %st, align 8, !tbaa !2
  store double %ld1, double* %st_idx1, align 8, !tbaa !2
  store double %ld0, double* %st_idx2, align 8, !tbaa !2
  store double %ld1, double* %st_idx3, align 8, !tbaa !2
  ret void
; CHECK-LABEL: @merge_double
; CHECK: movups	(%rsi), %xmm0
; CHECK-DAG: movups	%xmm0, (%rdi)
; CHECK-DAG: movups	%xmm0, 16(%rdi)
; CHECK: retq
}

define void @merge_loadstore_int(i64* noalias nocapture readonly %p, i64* noalias nocapture %q) local_unnamed_addr #0 {
entry:
  %0 = load i64, i64* %p, align 8, !tbaa !1
  %arrayidx1 = getelementptr inbounds i64, i64* %p, i64 1
  %1 = load i64, i64* %arrayidx1, align 8, !tbaa !1
  store i64 %0, i64* %q, align 8, !tbaa !1
  %arrayidx3 = getelementptr inbounds i64, i64* %q, i64 1
  store i64 %1, i64* %arrayidx3, align 8, !tbaa !1
  %arrayidx4 = getelementptr inbounds i64, i64* %q, i64 2
  store i64 %0, i64* %arrayidx4, align 8, !tbaa !1
  %arrayidx5 = getelementptr inbounds i64, i64* %q, i64 3
  store i64 %1, i64* %arrayidx5, align 8, !tbaa !1
  ret void
; CHECK-LABEL: @merge_loadstore_int
; CHECK: movups	(%rdi), %xmm0
; CHECK-DAG: movups	%xmm0, (%rsi)
; CHECK-DAG: movups	%xmm0, 16(%rsi)
; CHECK: retq
}

define i64 @merge_loadstore_int_with_extra_use(i64* noalias nocapture readonly %p, i64* noalias nocapture %q) local_unnamed_addr #0 {
entry:
  %0 = load i64, i64* %p, align 8, !tbaa !1
  %arrayidx1 = getelementptr inbounds i64, i64* %p, i64 1
  %1 = load i64, i64* %arrayidx1, align 8, !tbaa !1
  store i64 %0, i64* %q, align 8, !tbaa !1
  %arrayidx3 = getelementptr inbounds i64, i64* %q, i64 1
  store i64 %1, i64* %arrayidx3, align 8, !tbaa !1
  %arrayidx4 = getelementptr inbounds i64, i64* %q, i64 2
  store i64 %0, i64* %arrayidx4, align 8, !tbaa !1
  %arrayidx5 = getelementptr inbounds i64, i64* %q, i64 3
  store i64 %1, i64* %arrayidx5, align 8, !tbaa !1
  ret i64 %0
; CHECK-LABEL: @merge_loadstore_int_with_extra_use
; CHECK-DAG: movq	(%rdi), %rax
; CHECK-DAG: movups	(%rdi), %xmm0
; CHECK-DAG: movups	%xmm0, (%rsi)
; CHECK-DAG: movups	%xmm0, 16(%rsi)
; CHECK: retq

}

attributes #0 = { "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" }


!0 = !{!"clang version 5.0.0 (trunk 296467) (llvm/trunk 296476)"}
!1 = !{!2, !2, i64 0}
!2 = !{!"double", !3, i64 0}
!3 = !{!"omnipotent char", !4, i64 0}
!4 = !{!"Simple C/C++ TBAA"}
