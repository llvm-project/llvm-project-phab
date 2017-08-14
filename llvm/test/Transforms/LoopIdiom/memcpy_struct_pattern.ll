; RUN: opt -loop-idiom -loop-deletion < %s -S | FileCheck %s

; ModuleID = '<stdin>'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc_linux"

%struct.foo = type { i32, i32 }
%struct.foo1 = type { i32, i32, i32 }

; Function Attrs: norecurse nounwind uwtable
define void @bar1(%struct.foo* noalias nocapture %f, %struct.foo* noalias nocapture readonly %g, i32 %n) local_unnamed_addr #0 {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds %struct.foo, %struct.foo* %g, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 4, !tbaa !2
  %a3 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !2
  %b = getelementptr inbounds %struct.foo, %struct.foo* %g, i64 %indvars.iv, i32 1
  %1 = load i32, i32* %b, align 4, !tbaa !7
  %b8 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 1
  store i32 %1, i32* %b8, align 4, !tbaa !7
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %wide.trip.count = zext i32 %n to i64
  %exitcond = icmp ne i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.body, label %for.end.loopexit

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  ret void
; CHECK-LABEL: @bar1(
; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: norecurse nounwind uwtable
define void @bar2(%struct.foo* noalias nocapture %f, %struct.foo* noalias nocapture readonly %g, i32 %n) local_unnamed_addr #0 {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds %struct.foo, %struct.foo* %g, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 4, !tbaa !2
  %a3 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !2
  %b = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 1
  store i32 %0, i32* %b, align 4, !tbaa !7
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %wide.trip.count = zext i32 %n to i64
  %exitcond = icmp ne i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.body, label %for.end.loopexit

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  ret void
; CHECK-LABEL: @bar2(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar3(%struct.foo* noalias nocapture %f, %struct.foo* noalias nocapture readonly %g, i32 %n) local_unnamed_addr #0 {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next, %for.body ]
  %b = getelementptr inbounds %struct.foo, %struct.foo* %g, i64 %indvars.iv, i32 1
  %0 = load i32, i32* %b, align 4, !tbaa !7
  %a = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a, align 4, !tbaa !2
  %a5 = getelementptr inbounds %struct.foo, %struct.foo* %g, i64 %indvars.iv, i32 0
  %1 = load i32, i32* %a5, align 4, !tbaa !2
  %b8 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 1
  store i32 %1, i32* %b8, align 4, !tbaa !7
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %wide.trip.count = zext i32 %n to i64
  %exitcond = icmp ne i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.body, label %for.end.loopexit

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  ret void
; CHECK-LABEL: @bar3(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar4(%struct.foo* noalias nocapture %f, %struct.foo* noalias nocapture readonly %g, i32 %n) local_unnamed_addr #0 {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds %struct.foo, %struct.foo* %g, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 4, !tbaa !2
  %a3 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !2
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %wide.trip.count = zext i32 %n to i64
  %exitcond = icmp ne i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.body, label %for.end.loopexit

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  ret void
; CHECK-LABEL: @bar4(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar5(i32* noalias nocapture %f, i32* noalias nocapture readonly %g, i32 %n) local_unnamed_addr #0 {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next, %for.body ]
  %arrayidx = getelementptr inbounds i32, i32* %g, i64 %indvars.iv
  %0 = load i32, i32* %arrayidx, align 4, !tbaa !8
  %arrayidx2 = getelementptr inbounds i32, i32* %f, i64 %indvars.iv
  store i32 %0, i32* %arrayidx2, align 4, !tbaa !8
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %wide.trip.count = zext i32 %n to i64
  %exitcond = icmp ne i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.body, label %for.end.loopexit

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  ret void
; CHECK-LABEL: @bar5(
; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: norecurse nounwind uwtable
define void @bar6(%struct.foo1* noalias nocapture %f, %struct.foo1* noalias nocapture readonly %g, i32 %n) local_unnamed_addr #0 {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds %struct.foo1, %struct.foo1* %g, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 4, !tbaa !9
  %a3 = getelementptr inbounds %struct.foo1, %struct.foo1* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !9
  %b = getelementptr inbounds %struct.foo1, %struct.foo1* %g, i64 %indvars.iv, i32 1
  %1 = load i32, i32* %b, align 4, !tbaa !11
  %b8 = getelementptr inbounds %struct.foo1, %struct.foo1* %f, i64 %indvars.iv, i32 1
  store i32 %1, i32* %b8, align 4, !tbaa !11
  %c = getelementptr inbounds %struct.foo1, %struct.foo1* %g, i64 %indvars.iv, i32 2
  %2 = load i32, i32* %c, align 4, !tbaa !12
  %c13 = getelementptr inbounds %struct.foo1, %struct.foo1* %f, i64 %indvars.iv, i32 2
  store i32 %2, i32* %c13, align 4, !tbaa !12
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %wide.trip.count = zext i32 %n to i64
  %exitcond = icmp ne i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.body, label %for.end.loopexit

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  ret void
; CHECK-LABEL: @bar6(
; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: norecurse nounwind uwtable
define void @bar7(%struct.foo1* noalias nocapture %f, %struct.foo1* noalias nocapture readonly %g, i32 %n) local_unnamed_addr #0 {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds %struct.foo1, %struct.foo1* %g, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 4, !tbaa !9
  %a3 = getelementptr inbounds %struct.foo1, %struct.foo1* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !9
  %b = getelementptr inbounds %struct.foo1, %struct.foo1* %g, i64 %indvars.iv, i32 1
  %1 = load i32, i32* %b, align 4, !tbaa !11
  %b8 = getelementptr inbounds %struct.foo1, %struct.foo1* %f, i64 %indvars.iv, i32 1
  store i32 %1, i32* %b8, align 4, !tbaa !11
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %wide.trip.count = zext i32 %n to i64
  %exitcond = icmp ne i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.body, label %for.end.loopexit

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  ret void
; CHECK-LABEL: @bar7(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar8(%struct.foo1* noalias nocapture %f, %struct.foo1* noalias nocapture readonly %g, i32 %n) local_unnamed_addr #0 {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds %struct.foo1, %struct.foo1* %g, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 4, !tbaa !9
  %a3 = getelementptr inbounds %struct.foo1, %struct.foo1* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !9
  %c = getelementptr inbounds %struct.foo1, %struct.foo1* %g, i64 %indvars.iv, i32 2
  %1 = load i32, i32* %c, align 4, !tbaa !12
  %c8 = getelementptr inbounds %struct.foo1, %struct.foo1* %f, i64 %indvars.iv, i32 2
  store i32 %1, i32* %c8, align 4, !tbaa !12
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %wide.trip.count = zext i32 %n to i64
  %exitcond = icmp ne i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.body, label %for.end.loopexit

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  ret void
; CHECK-LABEL: @bar8(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

attributes #0 = { norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="slm" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 5.0.0 (https://github.com/llvm-mirror/clang.git 6cfb5bf41823be28bca09fe72dd3d4b83f4e1be8) (https://github.com/llvm-mirror/llvm.git 5ed4492ece68252fe8309cf11ea7e7105eabdeaf)"}
!2 = !{!3, !4, i64 0}
!3 = !{!"foo", !4, i64 0, !4, i64 4}
!4 = !{!"int", !5, i64 0}
!5 = !{!"omnipotent char", !6, i64 0}
!6 = !{!"Simple C/C++ TBAA"}
!7 = !{!3, !4, i64 4}
!8 = !{!4, !4, i64 0}
!9 = !{!10, !4, i64 0}
!10 = !{!"foo1", !4, i64 0, !4, i64 4, !4, i64 8}
!11 = !{!10, !4, i64 4}
!12 = !{!10, !4, i64 8}
