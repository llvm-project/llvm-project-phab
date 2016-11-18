; RUN: opt < %s  -loop-vectorize -force-vector-interleave=1 -force-vector-width=4 -S -debug-only=loop-vectorize 2>&1 | FileCheck %s
; REQUIRES: asserts

; CHECK: Checking a loop in "high_dynamic"
; CHECK: We can vectorize this loop
; CHECK: Checking a loop in "low_dynamic"
; CHECK: Found a loop with a very small trip count.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK-LABEL: @high_dynamic
; CHECK: fadd <4 x float>
define void @high_dynamic(float* nocapture %a, i32 %k) !prof !0 {
entry:
  br label %for.body

for.body:                                         ; preds = %for.body, %entry
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %arrayidx = getelementptr inbounds float, float* %a, i64 %indvars.iv
  %0 = load float, float* %arrayidx, align 4
  %add = fadd float %0, 1.000000e+00
  store float %add, float* %arrayidx, align 4
  %indvars.iv.next = add i64 %indvars.iv, 1
  %lftr.wideiv = trunc i64 %indvars.iv.next to i32
  %exitcond = icmp eq i32 %lftr.wideiv, %k
  br i1 %exitcond, label %for.end, label %for.body, !prof !1

for.end:                                          ; preds = %for.body
  ret void
}

; CHECK-LABEL: @low_dynamic
; CHECK-NOT: <4 x float>
define void @low_dynamic(float* nocapture %a, i32 %k) !prof !0 {
entry:
  br label %for.body

for.body:                                         ; preds = %for.body, %entry
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %arrayidx = getelementptr inbounds float, float* %a, i64 %indvars.iv
  %0 = load float, float* %arrayidx, align 4
  %add = fadd float %0, 1.000000e+00
  store float %add, float* %arrayidx, align 4
  %indvars.iv.next = add i64 %indvars.iv, 1
  %lftr.wideiv = trunc i64 %indvars.iv.next to i32
  %exitcond = icmp eq i32 %lftr.wideiv, %k
  br i1 %exitcond, label %for.end, label %for.body, !prof !2

for.end:                                          ; preds = %for.body
  ret void
}


!0 = !{!"function_entry_count", i64 1}
!1 = !{!"branch_weights", i32 1001, i32 400001}
!2 = !{!"branch_weights", i32 1001, i32 4001}
