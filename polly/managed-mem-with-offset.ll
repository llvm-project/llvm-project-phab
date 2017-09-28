; RUN: opt %loadPolly -polly-scops \
; RUN: -analyze < %s | FileCheck %s --check-prefix=SCOP

; RUN: opt %loadPolly -polly-codegen-ppcg \
; RUN: -S -polly-acc-codegen-managed-memory \
; RUN: < %s | FileCheck %s --check-prefix=HOST-IR
;
; REQUIRES: pollyacc

; We used to generate an offset computation twice, creating incorrect
; IR. Fix this to only compute offsets once. This is a regression test.

; SCOP: Function: f
; SCOP-NEXT: Region: %entry.split---%for.end
; SCOP-NEXT: Max Loop Depth:  1

; We should NOT have a GEP with an offset and then an undo of the offset.
; HOST-IR-NOT:  %9 = getelementptr i32, i32* %arr, i64 100
; HOST-IR-NOT:  %10 = bitcast i32* %9 to i8*
; HOST-IR-NOT:  %11 = bitcast i8* %10 to i32*
; HOST-IR-NOT:  %12 = getelementptr i32, i32* %11, i64 -100
; HOST-IR-NOT:  %13 = bitcast i32* %12 to i8*

; HOST-IR:  %9 = bitcast i32* %arr to i8*
; HOST-IR:  %10 = bitcast i8* %9 to i32*
; HOST-IR:  %11 = getelementptr i32, i32* %10, i64 -100
; HOST-IR:  %12 = bitcast i32* %11 to i8*

target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.12.0"

define void @f(i32* %arr, i32 %N) {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %tmp = sext i32 %N to i64
  %cmp1 = icmp sgt i32 %N, 0
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

for.body.lr.ph:                                   ; preds = %entry.split
  br label %for.body

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %indvars.iv2 = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %tmp3 = add nuw nsw i64 %indvars.iv2, 100
  %arrayidx = getelementptr inbounds i32, i32* %arr, i64 %tmp3
  %tmp4 = load i32, i32* %arrayidx, align 4, !tbaa !3
  %add1 = add nsw i32 %tmp4, 42
  store i32 %add1, i32* %arrayidx, align 4, !tbaa !3
  %indvars.iv.next = add nuw nsw i64 %indvars.iv2, 1
  %exitcond = icmp ne i64 %indvars.iv.next, %tmp
  br i1 %exitcond, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry.split
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #0

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #0

attributes #0 = { argmemonly nounwind }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{!"clang version 6.0.0 (http://llvm.org/git/clang.git a2ce4ccdf0d85af0048a3d80644af4fcee34c007) (http://llvm.org/git/llvm.git 75f9fda9e19662a3f06732843da79e8a5b9c448c)"}
!3 = !{!4, !4, i64 0}
!4 = !{!"int", !5, i64 0}
!5 = !{!"omnipotent char", !6, i64 0}
!6 = !{!"Simple C/C++ TBAA"}
