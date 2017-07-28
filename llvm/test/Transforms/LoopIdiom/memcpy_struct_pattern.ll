; RUN: opt -globals-aa -loop-idiom < %s -S | FileCheck %s

; ModuleID = '<stdin>'
source_filename = "../Newmemcpytest/test17.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.foo = type { i32, i32 }
%struct.foo1 = type { i32 }
%struct.foo3 = type { i32, i32, i32 }

@bar3.g = private unnamed_addr constant [14 x %struct.foo] [%struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }, %struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }], align 16
@bar4.g = private unnamed_addr constant [14 x %struct.foo1] [%struct.foo1 { i32 2 }, %struct.foo1 { i32 4 }, %struct.foo1 { i32 5 }, %struct.foo1 { i32 6 }, %struct.foo1 { i32 9 }, %struct.foo1 { i32 5 }, %struct.foo1 { i32 1 }, %struct.foo1 { i32 2 }, %struct.foo1 { i32 3 }, %struct.foo1 { i32 3 }, %struct.foo1 { i32 5 }, %struct.foo1 { i32 7 }, %struct.foo1 { i32 8 }, %struct.foo1 { i32 19 }], align 16
@bar5.g = private unnamed_addr constant [14 x %struct.foo] [%struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }, %struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }], align 16
@bar6.g = private unnamed_addr constant [14 x %struct.foo] [%struct.foo { i32 12, i32 3 }, %struct.foo { i32 3, i32 54 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }, %struct.foo { i32 2, i32 53 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 74, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }], align 16
@bar7.g = private unnamed_addr constant [14 x %struct.foo] [%struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }, %struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }], align 16
@bar8.g = private unnamed_addr constant [14 x %struct.foo] [%struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }, %struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }], align 16
@bar9.g = private unnamed_addr constant [14 x %struct.foo3] [%struct.foo3 { i32 2, i32 3, i32 4 }, %struct.foo3 { i32 3, i32 4, i32 1 }, %struct.foo3 { i32 1, i32 8, i32 34 }, %struct.foo3 { i32 3, i32 2, i32 21 }, %struct.foo3 { i32 4, i32 5, i32 78 }, %struct.foo3 { i32 4, i32 5, i32 123 }, %struct.foo3 { i32 1, i32 2, i32 3 }, %struct.foo3 { i32 2, i32 3, i32 34 }, %struct.foo3 { i32 3, i32 4, i32 34 }, %struct.foo3 { i32 1, i32 8, i32 53 }, %struct.foo3 { i32 3, i32 2, i32 87 }, %struct.foo3 { i32 4, i32 5, i32 43 }, %struct.foo3 { i32 4, i32 5, i32 34 }, %struct.foo3 { i32 1, i32 2, i32 45 }], align 16
@bar10.g = private unnamed_addr constant [14 x %struct.foo3] [%struct.foo3 { i32 2, i32 3, i32 4 }, %struct.foo3 { i32 3, i32 4, i32 1 }, %struct.foo3 { i32 1, i32 8, i32 34 }, %struct.foo3 { i32 3, i32 2, i32 21 }, %struct.foo3 { i32 4, i32 5, i32 78 }, %struct.foo3 { i32 4, i32 5, i32 123 }, %struct.foo3 { i32 1, i32 2, i32 3 }, %struct.foo3 { i32 2, i32 3, i32 34 }, %struct.foo3 { i32 3, i32 4, i32 34 }, %struct.foo3 { i32 1, i32 8, i32 53 }, %struct.foo3 { i32 3, i32 2, i32 87 }, %struct.foo3 { i32 4, i32 5, i32 43 }, %struct.foo3 { i32 4, i32 5, i32 34 }, %struct.foo3 { i32 1, i32 2, i32 45 }], align 16
@bar11.g = private unnamed_addr constant [14 x %struct.foo3] [%struct.foo3 { i32 2, i32 3, i32 4 }, %struct.foo3 { i32 3, i32 4, i32 1 }, %struct.foo3 { i32 1, i32 8, i32 34 }, %struct.foo3 { i32 3, i32 2, i32 21 }, %struct.foo3 { i32 4, i32 5, i32 78 }, %struct.foo3 { i32 4, i32 5, i32 123 }, %struct.foo3 { i32 1, i32 2, i32 3 }, %struct.foo3 { i32 2, i32 3, i32 34 }, %struct.foo3 { i32 3, i32 4, i32 34 }, %struct.foo3 { i32 1, i32 8, i32 53 }, %struct.foo3 { i32 3, i32 2, i32 87 }, %struct.foo3 { i32 4, i32 5, i32 43 }, %struct.foo3 { i32 4, i32 5, i32 34 }, %struct.foo3 { i32 1, i32 2, i32 45 }], align 16
@bar12.g = private unnamed_addr constant [14 x %struct.foo3] [%struct.foo3 { i32 2, i32 3, i32 4 }, %struct.foo3 { i32 3, i32 4, i32 1 }, %struct.foo3 { i32 1, i32 8, i32 34 }, %struct.foo3 { i32 3, i32 2, i32 21 }, %struct.foo3 { i32 4, i32 5, i32 78 }, %struct.foo3 { i32 4, i32 5, i32 123 }, %struct.foo3 { i32 1, i32 2, i32 3 }, %struct.foo3 { i32 2, i32 3, i32 34 }, %struct.foo3 { i32 3, i32 4, i32 34 }, %struct.foo3 { i32 1, i32 8, i32 53 }, %struct.foo3 { i32 3, i32 2, i32 87 }, %struct.foo3 { i32 4, i32 5, i32 43 }, %struct.foo3 { i32 4, i32 5, i32 34 }, %struct.foo3 { i32 1, i32 2, i32 45 }], align 16
@bar13.g = private unnamed_addr constant [14 x %struct.foo] [%struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }, %struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }], align 16
@bar14.g = private unnamed_addr constant [14 x %struct.foo1] [%struct.foo1 { i32 2 }, %struct.foo1 { i32 4 }, %struct.foo1 { i32 5 }, %struct.foo1 { i32 6 }, %struct.foo1 { i32 9 }, %struct.foo1 { i32 5 }, %struct.foo1 { i32 1 }, %struct.foo1 { i32 2 }, %struct.foo1 { i32 3 }, %struct.foo1 { i32 3 }, %struct.foo1 { i32 5 }, %struct.foo1 { i32 7 }, %struct.foo1 { i32 8 }, %struct.foo1 { i32 19 }], align 16
@bar15.g = private unnamed_addr constant [14 x %struct.foo] [%struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }, %struct.foo { i32 2, i32 3 }, %struct.foo { i32 3, i32 4 }, %struct.foo { i32 1, i32 8 }, %struct.foo { i32 3, i32 2 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 4, i32 5 }, %struct.foo { i32 1, i32 2 }], align 16

; Function Attrs: nounwind uwtable
define void @bar1(%struct.foo* nocapture %f) local_unnamed_addr #0 {
entry:
  %g = alloca [14 x %struct.foo], align 16
  %0 = bitcast [14 x %struct.foo]* %g to i8*
  call void @llvm.lifetime.start.p0i8(i64 112, i8* nonnull %0) #3
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 13, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* %g, i64 0, i64 %indvars.iv, i32 0
  %1 = load i32, i32* %a, align 8, !tbaa !1
  %a3 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 0
  store i32 %1, i32* %a3, align 4, !tbaa !1
  %indvars.iv.next = add nsw i64 %indvars.iv, -1
  %cmp = icmp sgt i64 %indvars.iv, 0
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  call void @llvm.lifetime.end.p0i8(i64 112, i8* nonnull %0) #3
  ret void
; CHECK-LABEL: @bar1(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #1

; Function Attrs: nounwind uwtable
define void @bar2(%struct.foo1* nocapture %f) local_unnamed_addr #0 {
entry:
  %g = alloca [14 x %struct.foo1], align 16
  %0 = bitcast [14 x %struct.foo1]* %g to i8*
  call void @llvm.lifetime.start.p0i8(i64 56, i8* nonnull %0) #3
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 13, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo1], [14 x %struct.foo1]* %g, i64 0, i64 %indvars.iv, i32 0
  %1 = load i32, i32* %a, align 4, !tbaa !6
  %a3 = getelementptr inbounds %struct.foo1, %struct.foo1* %f, i64 %indvars.iv, i32 0
  store i32 %1, i32* %a3, align 4, !tbaa !6
  %indvars.iv.next = add nsw i64 %indvars.iv, -1
  %cmp = icmp sgt i64 %indvars.iv, 0
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  call void @llvm.lifetime.end.p0i8(i64 56, i8* nonnull %0) #3
  ret void
; CHECK-LABEL: @bar2(
; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: norecurse nounwind uwtable
define void @bar3(%struct.foo* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* @bar3.g, i64 0, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 8, !tbaa !1
  %a3 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !1
  %b = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* @bar3.g, i64 0, i64 %indvars.iv, i32 1
  %1 = load i32, i32* %b, align 4, !tbaa !8
  %b8 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 1
  store i32 %1, i32* %b8, align 4, !tbaa !8
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp ne i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar3(
; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i32, i1) #1

; Function Attrs: norecurse nounwind uwtable
define void @bar4(%struct.foo1* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo1], [14 x %struct.foo1]* @bar4.g, i64 0, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 4, !tbaa !6
  %a3 = getelementptr inbounds %struct.foo1, %struct.foo1* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !6
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp ne i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar4(
; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: norecurse nounwind uwtable
define void @bar5(%struct.foo* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* @bar5.g, i64 0, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 8, !tbaa !1
  %b = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 1
  store i32 %0, i32* %b, align 4, !tbaa !8
  %b5 = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* @bar5.g, i64 0, i64 %indvars.iv, i32 1
  %1 = load i32, i32* %b5, align 4, !tbaa !8
  %a8 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 0
  store i32 %1, i32* %a8, align 4, !tbaa !1
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp ne i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar5(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar6(%struct.foo* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 13, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* @bar6.g, i64 0, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 8, !tbaa !1
  %a3 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !1
  %b = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* @bar6.g, i64 0, i64 %indvars.iv, i32 1
  %1 = load i32, i32* %b, align 4, !tbaa !8
  %b8 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 1
  store i32 %1, i32* %b8, align 4, !tbaa !8
  %indvars.iv.next = add nsw i64 %indvars.iv, -1
  %cmp = icmp sgt i64 %indvars.iv, 0
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar6(
; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: norecurse nounwind uwtable
define void @bar7(%struct.foo* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* @bar7.g, i64 0, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 8, !tbaa !1
  %a3 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !1
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp ne i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar7(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar8(%struct.foo* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* @bar8.g, i64 0, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 8, !tbaa !1
  %b = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 1
  store i32 %0, i32* %b, align 4, !tbaa !8
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp ne i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar8(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar9(%struct.foo3* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 13, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo3], [14 x %struct.foo3]* @bar9.g, i64 0, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 4, !tbaa !9
  %a3 = getelementptr inbounds %struct.foo3, %struct.foo3* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !9
  %b = getelementptr inbounds [14 x %struct.foo3], [14 x %struct.foo3]* @bar9.g, i64 0, i64 %indvars.iv, i32 1
  %1 = load i32, i32* %b, align 4, !tbaa !11
  %b8 = getelementptr inbounds %struct.foo3, %struct.foo3* %f, i64 %indvars.iv, i32 1
  store i32 %1, i32* %b8, align 4, !tbaa !11
  %c = getelementptr inbounds [14 x %struct.foo3], [14 x %struct.foo3]* @bar9.g, i64 0, i64 %indvars.iv, i32 2
  %2 = load i32, i32* %c, align 4, !tbaa !12
  %c13 = getelementptr inbounds %struct.foo3, %struct.foo3* %f, i64 %indvars.iv, i32 2
  store i32 %2, i32* %c13, align 4, !tbaa !12
  %indvars.iv.next = add nsw i64 %indvars.iv, -1
  %cmp = icmp sgt i64 %indvars.iv, 0
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar9(
; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: norecurse nounwind uwtable
define void @bar10(%struct.foo3* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 13, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo3], [14 x %struct.foo3]* @bar10.g, i64 0, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 4, !tbaa !9
  %a3 = getelementptr inbounds %struct.foo3, %struct.foo3* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !9
  %b = getelementptr inbounds [14 x %struct.foo3], [14 x %struct.foo3]* @bar10.g, i64 0, i64 %indvars.iv, i32 1
  %1 = load i32, i32* %b, align 4, !tbaa !11
  %b8 = getelementptr inbounds %struct.foo3, %struct.foo3* %f, i64 %indvars.iv, i32 1
  store i32 %1, i32* %b8, align 4, !tbaa !11
  %indvars.iv.next = add nsw i64 %indvars.iv, -1
  %cmp = icmp sgt i64 %indvars.iv, 0
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar10(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar11(%struct.foo3* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 13, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo3], [14 x %struct.foo3]* @bar11.g, i64 0, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 4, !tbaa !9
  %a3 = getelementptr inbounds %struct.foo3, %struct.foo3* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !9
  %c = getelementptr inbounds [14 x %struct.foo3], [14 x %struct.foo3]* @bar11.g, i64 0, i64 %indvars.iv, i32 2
  %1 = load i32, i32* %c, align 4, !tbaa !12
  %c8 = getelementptr inbounds %struct.foo3, %struct.foo3* %f, i64 %indvars.iv, i32 2
  store i32 %1, i32* %c8, align 4, !tbaa !12
  %indvars.iv.next = add nsw i64 %indvars.iv, -1
  %cmp = icmp sgt i64 %indvars.iv, 0
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar11(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar12(%struct.foo3* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 13, %entry ], [ %indvars.iv.next, %for.body ]
  %a = getelementptr inbounds [14 x %struct.foo3], [14 x %struct.foo3]* @bar12.g, i64 0, i64 %indvars.iv, i32 0
  %0 = load i32, i32* %a, align 4, !tbaa !9
  %a3 = getelementptr inbounds %struct.foo3, %struct.foo3* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a3, align 4, !tbaa !9
  %b = getelementptr inbounds [14 x %struct.foo3], [14 x %struct.foo3]* @bar12.g, i64 0, i64 %indvars.iv, i32 1
  %1 = load i32, i32* %b, align 4, !tbaa !11
  %b8 = getelementptr inbounds %struct.foo3, %struct.foo3* %f, i64 %indvars.iv, i32 1
  store i32 %1, i32* %b8, align 4, !tbaa !11
  %2 = load i32, i32* %a, align 4, !tbaa !9
  %c = getelementptr inbounds %struct.foo3, %struct.foo3* %f, i64 %indvars.iv, i32 2
  store i32 %2, i32* %c, align 4, !tbaa !12
  %indvars.iv.next = add nsw i64 %indvars.iv, -1
  %cmp = icmp sgt i64 %indvars.iv, 0
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar12(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar13(%struct.foo* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %b = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* @bar13.g, i64 0, i64 %indvars.iv, i32 1
  %0 = load i32, i32* %b, align 4, !tbaa !8
  %a = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %indvars.iv, i32 0
  store i32 %0, i32* %a, align 4, !tbaa !1
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp ne i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar13(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar14(%struct.foo1* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %0 = sub nuw nsw i64 13, %indvars.iv
  %a = getelementptr inbounds [14 x %struct.foo1], [14 x %struct.foo1]* @bar14.g, i64 0, i64 %0, i32 0
  %1 = load i32, i32* %a, align 4, !tbaa !6
  %a3 = getelementptr inbounds %struct.foo1, %struct.foo1* %f, i64 %indvars.iv, i32 0
  store i32 %1, i32* %a3, align 4, !tbaa !6
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp ne i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar14(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: norecurse nounwind uwtable
define void @bar15(%struct.foo* nocapture %f) local_unnamed_addr #2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %0 = sub nuw nsw i64 13, %indvars.iv
  %a = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* @bar15.g, i64 0, i64 %0, i32 0
  %1 = load i32, i32* %a, align 8, !tbaa !1
  %a4 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %0, i32 0
  store i32 %1, i32* %a4, align 4, !tbaa !1
  %b = getelementptr inbounds [14 x %struct.foo], [14 x %struct.foo]* @bar15.g, i64 0, i64 %indvars.iv, i32 1
  %2 = load i32, i32* %b, align 4, !tbaa !8
  %b10 = getelementptr inbounds %struct.foo, %struct.foo* %f, i64 %0, i32 1
  store i32 %2, i32* %b10, align 4, !tbaa !8
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp ne i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
; CHECK-LABEL: @bar15(
; CHECK-NOT: call void @llvm.memcpy.p0i8.p0i8.i64
}

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }
attributes #2 = { norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 5.0.0 "}
!1 = !{!2, !3, i64 0}
!2 = !{!"foo", !3, i64 0, !3, i64 4}
!3 = !{!"int", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
!6 = !{!7, !3, i64 0}
!7 = !{!"foo1", !3, i64 0}
!8 = !{!2, !3, i64 4}
!9 = !{!10, !3, i64 0}
!10 = !{!"foo3", !3, i64 0, !3, i64 4, !3, i64 8}
!11 = !{!10, !3, i64 4}
!12 = !{!10, !3, i64 8}
