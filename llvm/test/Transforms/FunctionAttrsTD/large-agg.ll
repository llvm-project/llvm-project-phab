; RUN: opt -S -basicaa -functionattrs-td < %s | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.s = type { [65280 x i32] }
%struct.x = type opaque
%struct.x.0 = type opaque

; CHECK: define void @bar()

; Function Attrs: nounwind uwtable
define void @bar() #0 {
entry:
  %x = alloca %struct.s, align 32
  %y = alloca %struct.s, align 4
  %0 = bitcast %struct.s* %x to i8*
  call void @llvm.lifetime.start(i64 261120, i8* %0) #1
  %1 = bitcast %struct.s* %y to i8*
  call void @llvm.lifetime.start(i64 261120, i8* %1) #1
  br label %for.body

for.body:                                         ; preds = %for.body, %entry
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %arrayidx = getelementptr inbounds %struct.s* %x, i64 0, i32 0, i64 %indvars.iv
  %2 = trunc i64 %indvars.iv to i32
  store i32 %2, i32* %arrayidx, align 4, !tbaa !0
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 17800
  br i1 %exitcond, label %for.end, label %for.body

for.end:                                          ; preds = %for.body
  %3 = bitcast %struct.s* %y to %struct.x*
  call fastcc void @foo(%struct.s* %x, %struct.x* %3)
  call void @llvm.lifetime.end(i64 261120, i8* %1) #1
  call void @llvm.lifetime.end(i64 261120, i8* %0) #1
  ret void
}

; Function Attrs: nounwind
declare void @llvm.lifetime.start(i64, i8* nocapture) #1

; CHECK: define internal fastcc void @foo(%struct.s* noalias nocapture readonly align 32 dereferenceable(261120) %x, %struct.x* noalias %y)

; Function Attrs: noinline nounwind uwtable
define internal fastcc void @foo(%struct.s* nocapture readonly %x, %struct.x* %y) #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %for.body, %entry
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %sum.05 = phi i32 [ 0, %entry ], [ %add, %for.body ]
  %arrayidx = getelementptr inbounds %struct.s* %x, i64 0, i32 0, i64 %indvars.iv
  %0 = load i32* %arrayidx, align 4, !tbaa !0
  %add = add nsw i32 %0, %sum.05
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 16000
  br i1 %exitcond, label %for.end, label %for.body

for.end:                                          ; preds = %for.body
  %1 = bitcast %struct.x* %y to %struct.x.0*
  tail call void @goo(i32 %add, %struct.x.0* %1) #1
  ret void
}

; Function Attrs: nounwind
declare void @llvm.lifetime.end(i64, i8* nocapture) #1

declare void @goo(i32, %struct.x.0*)

attributes #0 = { nounwind uwtable }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind uwtable }

!0 = metadata !{metadata !1, metadata !1, i64 0}
!1 = metadata !{metadata !"int", metadata !2, i64 0}
!2 = metadata !{metadata !"omnipotent char", metadata !3, i64 0}
!3 = metadata !{metadata !"Simple C/C++ TBAA"}
