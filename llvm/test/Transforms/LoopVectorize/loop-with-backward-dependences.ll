; RUN: opt < %s -S -loop-vectorize-pred -branch-prob -block-freq -scalar-evolution -aa -loop-accesses -demanded-bits -lazy-branch-prob -lazy-block-freq -opt-remark-emitter -loop-vectorize   2>&1 | FileCheck %s
; ModuleID = 'test-disablellvm.ll'
source_filename = "../test2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc_linux"


; CHECK-LABEL: foo1
;
; CHECK:     vector.body
; CHECK:       %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ]
; CHECK:       getelementptr inbounds i32, i32* %b, i64 {{.*}}
; CHECK:       br i1 {{.*}}, label %middle.block, label %vector.body
;

; Function Attrs: norecurse nounwind uwtable
define i32 @foo1(i32 %n, i32* noalias nocapture %a, i32* noalias nocapture readonly %b, i32* noalias nocapture %m) local_unnamed_addr #0 {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext i32 %n to i64
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %for.body.preheader ]
  %arrayidx = getelementptr inbounds i32, i32* %b, i64 %indvars.iv
  %0 = load i32, i32* %arrayidx, align 4, !tbaa !2
  %arrayidx2 = getelementptr inbounds i32, i32* %a, i64 %indvars.iv
  store i32 %0, i32* %arrayidx2, align 4, !tbaa !2
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %arrayidx4 = getelementptr inbounds i32, i32* %a, i64 %indvars.iv.next
  %1 = load i32, i32* %arrayidx4, align 4, !tbaa !2
  %arrayidx6 = getelementptr inbounds i32, i32* %m, i64 %indvars.iv
  store i32 %1, i32* %arrayidx6, align 4, !tbaa !2
  %exitcond = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.end.loopexit, label %for.body

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  %arrayidx7 = getelementptr inbounds i32, i32* %m, i64 7
  %2 = load i32, i32* %arrayidx7, align 4, !tbaa !2
  ret i32 %2
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #1


; CHECK-LABEL: foo2
;
; CHECK:     vector.body
; CHECK:       %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ]
; CHECK:       getelementptr inbounds i32, i32* %b, i64 {{.*}}
; CHECK:       br i1 {{.*}}, label %middle.block, label %vector.body
;

; Function Attrs: norecurse nounwind uwtable
define i32 @foo2(i32 %n, i32* noalias nocapture %a, i32* noalias nocapture readonly %b, i32* noalias nocapture %m, i32* noalias nocapture readonly %c, i32* noalias nocapture %f, i32* noalias nocapture %d) local_unnamed_addr #0 {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext i32 %n to i64
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %for.body.preheader ]
  %arrayidx = getelementptr inbounds i32, i32* %b, i64 %indvars.iv
  %0 = load i32, i32* %arrayidx, align 4, !tbaa !2
  %arrayidx2 = getelementptr inbounds i32, i32* %a, i64 %indvars.iv
  store i32 %0, i32* %arrayidx2, align 4, !tbaa !2
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %arrayidx4 = getelementptr inbounds i32, i32* %a, i64 %indvars.iv.next
  %1 = load i32, i32* %arrayidx4, align 4, !tbaa !2
  %arrayidx6 = getelementptr inbounds i32, i32* %m, i64 %indvars.iv
  store i32 %1, i32* %arrayidx6, align 4, !tbaa !2
  %arrayidx8 = getelementptr inbounds i32, i32* %c, i64 %indvars.iv
  %2 = load i32, i32* %arrayidx8, align 4, !tbaa !2
  %arrayidx10 = getelementptr inbounds i32, i32* %f, i64 %indvars.iv
  store i32 %2, i32* %arrayidx10, align 4, !tbaa !2
  %arrayidx13 = getelementptr inbounds i32, i32* %f, i64 %indvars.iv.next
  %3 = load i32, i32* %arrayidx13, align 4, !tbaa !2
  %arrayidx15 = getelementptr inbounds i32, i32* %d, i64 %indvars.iv
  store i32 %3, i32* %arrayidx15, align 4, !tbaa !2
  %exitcond = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.end.loopexit, label %for.body

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  %arrayidx16 = getelementptr inbounds i32, i32* %m, i64 7
  %4 = load i32, i32* %arrayidx16, align 4, !tbaa !2
  ret i32 %4
}

; CHECK-LABEL: foo3
;
; CHECK:     vector.body
; CHECK:       %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ]
; CHECK:       getelementptr inbounds i32, i32* %b, i64 {{.*}}
; CHECK:       br i1 {{.*}}, label %middle.block, label %vector.body
;


; Function Attrs: norecurse nounwind uwtable
define i32 @foo3(i32 %n, i32* noalias nocapture %a, i32* noalias nocapture readonly %b, i32* noalias nocapture %m, i32* noalias nocapture %c) local_unnamed_addr #0 {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext i32 %n to i64
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %for.body.preheader ]
  %arrayidx = getelementptr inbounds i32, i32* %b, i64 %indvars.iv
  %0 = load i32, i32* %arrayidx, align 4, !tbaa !2
  %arrayidx2 = getelementptr inbounds i32, i32* %a, i64 %indvars.iv
  store i32 %0, i32* %arrayidx2, align 4, !tbaa !2
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %arrayidx4 = getelementptr inbounds i32, i32* %a, i64 %indvars.iv.next
  %1 = load i32, i32* %arrayidx4, align 4, !tbaa !2
  %arrayidx6 = getelementptr inbounds i32, i32* %m, i64 %indvars.iv
  store i32 %1, i32* %arrayidx6, align 4, !tbaa !2
  %2 = add nuw nsw i64 %indvars.iv, 4
  %arrayidx9 = getelementptr inbounds i32, i32* %a, i64 %2
  %3 = load i32, i32* %arrayidx9, align 4, !tbaa !2
  %arrayidx11 = getelementptr inbounds i32, i32* %c, i64 %indvars.iv
  store i32 %3, i32* %arrayidx11, align 4, !tbaa !2
  %exitcond = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.end.loopexit, label %for.body

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  %arrayidx12 = getelementptr inbounds i32, i32* %m, i64 7
  %4 = load i32, i32* %arrayidx12, align 4, !tbaa !2
  ret i32 %4
}

attributes #0 = { norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="slm" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind "target-cpu"="slm" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 5.0.0 (https://github.com/llvm-mirror/clang.git 6cfb5bf41823be28bca09fe72dd3d4b83f4e1be8) (https://github.com/llvm-mirror/llvm.git 71ff951212dc0f4da08d348cdc6f14997ecc7f96)"}
!2 = !{!3, !3, i64 0}
!3 = !{!"int", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
