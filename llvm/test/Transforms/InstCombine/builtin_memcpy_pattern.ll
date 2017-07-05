; RUN: opt -O2  < %s -S | FileCheck %s

; ModuleID = '../memcpyTest/for_builtin_memcpy_test.c'
source_filename = "../memcpyTest/for_builtin_memcpy_test.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@foo1.b = private unnamed_addr constant [8 x i32] [i32 3, i32 4, i32 5, i32 56, i32 6, i32 7, i32 78, i32 6], align 16
@foo2.b = private unnamed_addr constant [8 x i32] [i32 3, i32 4, i32 5, i32 56, i32 6, i32 7, i32 78, i32 6], align 16
@foo3.b = private unnamed_addr constant [8 x i32] [i32 3, i32 4, i32 5, i32 56, i32 6, i32 7, i32 78, i32 6], align 16
@foo4.b = private unnamed_addr constant [8 x i32] [i32 3, i32 4, i32 5, i32 56, i32 6, i32 7, i32 78, i32 6], align 16
@foo5.b = private unnamed_addr constant [8 x i32] [i32 3, i32 4, i32 5, i32 56, i32 6, i32 7, i32 78, i32 6], align 16
@foo6.b = private unnamed_addr constant [16 x i32] [i32 3, i32 4, i32 5, i32 56, i32 6, i32 7, i32 78, i32 6, i32 3, i32 4, i32 54, i32 5, i32 1, i32 2, i32 3, i32 3], align 16

; Function Attrs: nounwind uwtable
define void @bar(i8* %a, i8* %b) #0 {
entry:
  %a.addr = alloca i8*, align 8
  %b.addr = alloca i8*, align 8
  store i8* %a, i8** %a.addr, align 8, !tbaa !2
  store i8* %b, i8** %b.addr, align 8, !tbaa !2
  %0 = load i8*, i8** %a.addr, align 8, !tbaa !2
  %1 = load i8*, i8** %b.addr, align 8, !tbaa !2
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16, i32 1, i1 false)
  ret void
; CHECK-LABEL: @bar(
; CHECK: store
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i32, i1) #1

; Function Attrs: nounwind uwtable
define void @foo(i8* %a, i8* %b, i32 %n) #0 {
entry:
  %a.addr = alloca i8*, align 8
  %b.addr = alloca i8*, align 8
  %n.addr = alloca i32, align 4
  %i = alloca i32, align 4
  store i8* %a, i8** %a.addr, align 8, !tbaa !2
  store i8* %b, i8** %b.addr, align 8, !tbaa !2
  store i32 %n, i32* %n.addr, align 4, !tbaa !6
  %0 = bitcast i32* %i to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %0) #2
  store i32 0, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %1 = load i32, i32* %i, align 4, !tbaa !6
  %2 = load i32, i32* %n.addr, align 4, !tbaa !6
  %cmp = icmp ult i32 %1, %2
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %3 = load i8*, i8** %a.addr, align 8, !tbaa !2
  %4 = load i32, i32* %i, align 4, !tbaa !6
  %idx.ext = sext i32 %4 to i64
  %add.ptr = getelementptr inbounds i8, i8* %3, i64 %idx.ext
  %5 = load i8*, i8** %b.addr, align 8, !tbaa !2
  %6 = load i32, i32* %i, align 4, !tbaa !6
  %idx.ext1 = sext i32 %6 to i64
  %add.ptr2 = getelementptr inbounds i8, i8* %5, i64 %idx.ext1
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %add.ptr, i8* %add.ptr2, i64 16, i32 1, i1 false)
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %7 = load i32, i32* %i, align 4, !tbaa !6
  %inc = add nsw i32 %7, 1
  store i32 %inc, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %8 = bitcast i32* %i to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %8) #2
  ret void
; CHECK-LABEL: @foo(
; CHECK: store
; CHECK-NOT: @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #1

; Function Attrs: nounwind uwtable
define void @foo1(i32* %a, i32 %n) #0 {
entry:
  %a.addr = alloca i32*, align 8
  %n.addr = alloca i32, align 4
  %b = alloca [8 x i32], align 16
  %i = alloca i32, align 4
  store i32* %a, i32** %a.addr, align 8, !tbaa !2
  store i32 %n, i32* %n.addr, align 4, !tbaa !6
  %0 = bitcast [8 x i32]* %b to i8*
  call void @llvm.lifetime.start.p0i8(i64 32, i8* %0) #2
  %1 = bitcast [8 x i32]* %b to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %1, i8* bitcast ([8 x i32]* @foo1.b to i8*), i64 32, i32 16, i1 false)
  %2 = bitcast i32* %i to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %2) #2
  store i32 0, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %3 = load i32, i32* %i, align 4, !tbaa !6
  %4 = load i32, i32* %n.addr, align 4, !tbaa !6
  %cmp = icmp ult i32 %3, %4
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %5 = load i32*, i32** %a.addr, align 8, !tbaa !2
  %6 = load i32, i32* %i, align 4, !tbaa !6
  %idx.ext = sext i32 %6 to i64
  %add.ptr = getelementptr inbounds i32, i32* %5, i64 %idx.ext
  %7 = bitcast i32* %add.ptr to i8*
  %arraydecay = getelementptr inbounds [8 x i32], [8 x i32]* %b, i32 0, i32 0
  %8 = load i32, i32* %i, align 4, !tbaa !6
  %idx.ext1 = sext i32 %8 to i64
  %add.ptr2 = getelementptr inbounds i32, i32* %arraydecay, i64 %idx.ext1
  %9 = bitcast i32* %add.ptr2 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %7, i8* %9, i64 4, i32 4, i1 false)
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %10 = load i32, i32* %i, align 4, !tbaa !6
  %inc = add nsw i32 %10, 1
  store i32 %inc, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %11 = bitcast i32* %i to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %11) #2
  %12 = bitcast [8 x i32]* %b to i8*
  call void @llvm.lifetime.end.p0i8(i64 32, i8* %12) #2
  ret void
; CHECK-LABEL: @foo1(
; CHECK: @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: nounwind uwtable
define void @foo2(i32* %a, i32 %n) #0 {
entry:
  %a.addr = alloca i32*, align 8
  %n.addr = alloca i32, align 4
  %b = alloca [8 x i32], align 16
  %i = alloca i32, align 4
  store i32* %a, i32** %a.addr, align 8, !tbaa !2
  store i32 %n, i32* %n.addr, align 4, !tbaa !6
  %0 = bitcast [8 x i32]* %b to i8*
  call void @llvm.lifetime.start.p0i8(i64 32, i8* %0) #2
  %1 = bitcast [8 x i32]* %b to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %1, i8* bitcast ([8 x i32]* @foo2.b to i8*), i64 32, i32 16, i1 false)
  %2 = bitcast i32* %i to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %2) #2
  store i32 0, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %3 = load i32, i32* %i, align 4, !tbaa !6
  %4 = load i32, i32* %n.addr, align 4, !tbaa !6
  %cmp = icmp ult i32 %3, %4
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %5 = load i32*, i32** %a.addr, align 8, !tbaa !2
  %6 = load i32, i32* %i, align 4, !tbaa !6
  %idx.ext = sext i32 %6 to i64
  %add.ptr = getelementptr inbounds i32, i32* %5, i64 %idx.ext
  %7 = bitcast i32* %add.ptr to i8*
  %arraydecay = getelementptr inbounds [8 x i32], [8 x i32]* %b, i32 0, i32 0
  %8 = load i32, i32* %i, align 4, !tbaa !6
  %idx.ext1 = sext i32 %8 to i64
  %add.ptr2 = getelementptr inbounds i32, i32* %arraydecay, i64 %idx.ext1
  %9 = bitcast i32* %add.ptr2 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %7, i8* %9, i64 8, i32 4, i1 false)
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %10 = load i32, i32* %i, align 4, !tbaa !6
  %inc = add nsw i32 %10, 1
  store i32 %inc, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %11 = bitcast i32* %i to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %11) #2
  %12 = bitcast [8 x i32]* %b to i8*
  call void @llvm.lifetime.end.p0i8(i64 32, i8* %12) #2
  ret void
; CHECK-LABEL: @foo2(
; CHECK: store
; CHECK-NOT: @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: nounwind uwtable
define void @foo3(i32* %a, i32 %n) #0 {
entry:
  %a.addr = alloca i32*, align 8
  %n.addr = alloca i32, align 4
  %b = alloca [8 x i32], align 16
  %i = alloca i32, align 4
  store i32* %a, i32** %a.addr, align 8, !tbaa !2
  store i32 %n, i32* %n.addr, align 4, !tbaa !6
  %0 = bitcast [8 x i32]* %b to i8*
  call void @llvm.lifetime.start.p0i8(i64 32, i8* %0) #2
  %1 = bitcast [8 x i32]* %b to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %1, i8* bitcast ([8 x i32]* @foo3.b to i8*), i64 32, i32 16, i1 false)
  %2 = bitcast i32* %i to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %2) #2
  store i32 0, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %3 = load i32, i32* %i, align 4, !tbaa !6
  %4 = load i32, i32* %n.addr, align 4, !tbaa !6
  %cmp = icmp ult i32 %3, %4
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %5 = load i32*, i32** %a.addr, align 8, !tbaa !2
  %6 = load i32, i32* %i, align 4, !tbaa !6
  %mul = mul nsw i32 2, %6
  %idx.ext = sext i32 %mul to i64
  %add.ptr = getelementptr inbounds i32, i32* %5, i64 %idx.ext
  %7 = bitcast i32* %add.ptr to i8*
  %arraydecay = getelementptr inbounds [8 x i32], [8 x i32]* %b, i32 0, i32 0
  %8 = load i32, i32* %i, align 4, !tbaa !6
  %mul1 = mul nsw i32 2, %8
  %idx.ext2 = sext i32 %mul1 to i64
  %add.ptr3 = getelementptr inbounds i32, i32* %arraydecay, i64 %idx.ext2
  %9 = bitcast i32* %add.ptr3 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %7, i8* %9, i64 8, i32 4, i1 false)
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %10 = load i32, i32* %i, align 4, !tbaa !6
  %inc = add nsw i32 %10, 1
  store i32 %inc, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %11 = bitcast i32* %i to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %11) #2
  %12 = bitcast [8 x i32]* %b to i8*
  call void @llvm.lifetime.end.p0i8(i64 32, i8* %12) #2
  ret void
; CHECK-LABEL: @foo3(
; CHECK: @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: nounwind uwtable
define void @foo4(i32* %a, i32 %n) #0 {
entry:
  %a.addr = alloca i32*, align 8
  %n.addr = alloca i32, align 4
  %b = alloca [8 x i32], align 16
  %i = alloca i32, align 4
  store i32* %a, i32** %a.addr, align 8, !tbaa !2
  store i32 %n, i32* %n.addr, align 4, !tbaa !6
  %0 = bitcast [8 x i32]* %b to i8*
  call void @llvm.lifetime.start.p0i8(i64 32, i8* %0) #2
  %1 = bitcast [8 x i32]* %b to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %1, i8* bitcast ([8 x i32]* @foo4.b to i8*), i64 32, i32 16, i1 false)
  %2 = bitcast i32* %i to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %2) #2
  store i32 0, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %3 = load i32, i32* %i, align 4, !tbaa !6
  %4 = load i32, i32* %n.addr, align 4, !tbaa !6
  %cmp = icmp ult i32 %3, %4
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %5 = load i32*, i32** %a.addr, align 8, !tbaa !2
  %6 = load i32, i32* %i, align 4, !tbaa !6
  %mul = mul nsw i32 2, %6
  %idx.ext = sext i32 %mul to i64
  %add.ptr = getelementptr inbounds i32, i32* %5, i64 %idx.ext
  %7 = bitcast i32* %add.ptr to i8*
  %arraydecay = getelementptr inbounds [8 x i32], [8 x i32]* %b, i32 0, i32 0
  %8 = load i32, i32* %i, align 4, !tbaa !6
  %mul1 = mul nsw i32 2, %8
  %idx.ext2 = sext i32 %mul1 to i64
  %add.ptr3 = getelementptr inbounds i32, i32* %arraydecay, i64 %idx.ext2
  %9 = bitcast i32* %add.ptr3 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %7, i8* %9, i64 16, i32 4, i1 false)
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %10 = load i32, i32* %i, align 4, !tbaa !6
  %inc = add nsw i32 %10, 1
  store i32 %inc, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %11 = bitcast i32* %i to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %11) #2
  %12 = bitcast [8 x i32]* %b to i8*
  call void @llvm.lifetime.end.p0i8(i64 32, i8* %12) #2
  ret void
; CHECK-LABEL: @foo4(
; CHECK: store
; CHECK-NOT: @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: nounwind uwtable
define void @foo5(i32* %a, i32 %n) #0 {
entry:
  %a.addr = alloca i32*, align 8
  %n.addr = alloca i32, align 4
  %b = alloca [8 x i32], align 16
  %i = alloca i32, align 4
  store i32* %a, i32** %a.addr, align 8, !tbaa !2
  store i32 %n, i32* %n.addr, align 4, !tbaa !6
  %0 = bitcast [8 x i32]* %b to i8*
  call void @llvm.lifetime.start.p0i8(i64 32, i8* %0) #2
  %1 = bitcast [8 x i32]* %b to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %1, i8* bitcast ([8 x i32]* @foo5.b to i8*), i64 32, i32 16, i1 false)
  %2 = bitcast i32* %i to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %2) #2
  store i32 0, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %3 = load i32, i32* %i, align 4, !tbaa !6
  %4 = load i32, i32* %n.addr, align 4, !tbaa !6
  %cmp = icmp ult i32 %3, %4
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %5 = load i32*, i32** %a.addr, align 8, !tbaa !2
  %6 = load i32, i32* %i, align 4, !tbaa !6
  %mul = mul nsw i32 4, %6
  %idx.ext = sext i32 %mul to i64
  %add.ptr = getelementptr inbounds i32, i32* %5, i64 %idx.ext
  %7 = bitcast i32* %add.ptr to i8*
  %arraydecay = getelementptr inbounds [8 x i32], [8 x i32]* %b, i32 0, i32 0
  %8 = load i32, i32* %i, align 4, !tbaa !6
  %mul1 = mul nsw i32 4, %8
  %idx.ext2 = sext i32 %mul1 to i64
  %add.ptr3 = getelementptr inbounds i32, i32* %arraydecay, i64 %idx.ext2
  %9 = bitcast i32* %add.ptr3 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %7, i8* %9, i64 16, i32 4, i1 false)
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %10 = load i32, i32* %i, align 4, !tbaa !6
  %inc = add nsw i32 %10, 1
  store i32 %inc, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %11 = bitcast i32* %i to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %11) #2
  %12 = bitcast [8 x i32]* %b to i8*
  call void @llvm.lifetime.end.p0i8(i64 32, i8* %12) #2
  ret void
; CHECK-LABEL: @foo5(
; CHECK: @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: nounwind uwtable
define void @foo6(i32* %a, i32 %n) #0 {
entry:
  %a.addr = alloca i32*, align 8
  %n.addr = alloca i32, align 4
  %b = alloca [16 x i32], align 16
  %i = alloca i32, align 4
  store i32* %a, i32** %a.addr, align 8, !tbaa !2
  store i32 %n, i32* %n.addr, align 4, !tbaa !6
  %0 = bitcast [16 x i32]* %b to i8*
  call void @llvm.lifetime.start.p0i8(i64 64, i8* %0) #2
  %1 = bitcast [16 x i32]* %b to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %1, i8* bitcast ([16 x i32]* @foo6.b to i8*), i64 64, i32 16, i1 false)
  %2 = bitcast i32* %i to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %2) #2
  store i32 0, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %3 = load i32, i32* %i, align 4, !tbaa !6
  %4 = load i32, i32* %n.addr, align 4, !tbaa !6
  %cmp = icmp ult i32 %3, %4
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %5 = load i32*, i32** %a.addr, align 8, !tbaa !2
  %6 = load i32, i32* %i, align 4, !tbaa !6
  %mul = mul nsw i32 8, %6
  %idx.ext = sext i32 %mul to i64
  %add.ptr = getelementptr inbounds i32, i32* %5, i64 %idx.ext
  %7 = bitcast i32* %add.ptr to i8*
  %arraydecay = getelementptr inbounds [16 x i32], [16 x i32]* %b, i32 0, i32 0
  %8 = load i32, i32* %i, align 4, !tbaa !6
  %mul1 = mul nsw i32 8, %8
  %idx.ext2 = sext i32 %mul1 to i64
  %add.ptr3 = getelementptr inbounds i32, i32* %arraydecay, i64 %idx.ext2
  %9 = bitcast i32* %add.ptr3 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %7, i8* %9, i64 32, i32 4, i1 false)
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %10 = load i32, i32* %i, align 4, !tbaa !6
  %inc = add nsw i32 %10, 1
  store i32 %inc, i32* %i, align 4, !tbaa !6
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %11 = bitcast i32* %i to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %11) #2
  %12 = bitcast [16 x i32]* %b to i8*
  call void @llvm.lifetime.end.p0i8(i64 64, i8* %12) #2
  ret void
; CHECK-LABEL: @foo6(
; CHECK: @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }
attributes #2 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 5.0.0 (https://github.com/llvm-mirror/clang.git 6cfb5bf41823be28bca09fe72dd3d4b83f4e1be8) (https://github.com/llvm-mirror/llvm.git 26c55932a492b686246e1f5cc85f916622eff8fe)"}
!2 = !{!3, !3, i64 0}
!3 = !{!"any pointer", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
!6 = !{!7, !7, i64 0}
!7 = !{!"int", !4, i64 0}
