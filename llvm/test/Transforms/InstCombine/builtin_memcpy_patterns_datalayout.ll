; RUN: opt -instcombine  < %s -S | FileCheck %s

; ModuleID = 'builtin_memcpy.ll'
source_filename = "../memcpyTest/for_builtin_memcpy_test1.c"
target datalayout = "E-p:64:64:64-a0:0:8-f32:32:32-f64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-v64:64:64-v128:128:128"
target triple = "x86_64-unknown-linux-gnu"

@b = local_unnamed_addr global [16 x i32] [i32 3, i32 4, i32 5, i32 56, i32 6, i32 7, i32 78, i32 6, i32 3, i32 4, i32 54, i32 5, i32 1, i32 2, i32 3, i32 3], align 16

; Function Attrs: nounwind uwtable
define void @foo(i8* %a, i8* %b) local_unnamed_addr #0 {
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %a, i8* %b, i64 16, i32 1, i1 false)
  ret void
; CHECK-LABEL: @foo(
; CHECK-NOT: store
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i32, i1) #1

; Function Attrs: nounwind uwtable
define void @foo1(i32* %a, i32 %n) local_unnamed_addr #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %cmp = icmp ult i32 %i.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %mul = mul nsw i32 2, %i.0
  %idx.ext = sext i32 %mul to i64
  %add.ptr = getelementptr inbounds i32, i32* %a, i64 %idx.ext
  %0 = bitcast i32* %add.ptr to i8*
  %add.ptr3 = getelementptr inbounds i32, i32* getelementptr inbounds ([16 x i32], [16 x i32]* @b, i32 0, i32 0), i64 %idx.ext
  %1 = bitcast i32* %add.ptr3 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 8, i32 4, i1 false)
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
; CHECK-LABEL: @foo1(
; CHECK: store
; CHECK-NOT: @llvm.memcpy.p0i8.p0i8.i64
}

; Function Attrs: nounwind uwtable
define void @foo2(i32* %a, i32 %n) local_unnamed_addr #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %cmp = icmp ult i32 %i.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %mul = mul nsw i32 4, %i.0
  %idx.ext = sext i32 %mul to i64
  %add.ptr = getelementptr inbounds i32, i32* %a, i64 %idx.ext
  %0 = bitcast i32* %add.ptr to i8*
  %add.ptr3 = getelementptr inbounds i32, i32* getelementptr inbounds ([16 x i32], [16 x i32]* @b, i32 0, i32 0), i64 %idx.ext
  %1 = bitcast i32* %add.ptr3 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16, i32 4, i1 false)
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
; CHECK-LABEL: @foo2(
; CHECK: @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

; Function Attrs: nounwind uwtable
define void @foo3(i32* %a, i32 %n) local_unnamed_addr #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %cmp = icmp ult i32 %i.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %mul = mul nsw i32 8, %i.0
  %idx.ext = sext i32 %mul to i64
  %add.ptr = getelementptr inbounds i32, i32* %a, i64 %idx.ext
  %0 = bitcast i32* %add.ptr to i8*
  %add.ptr3 = getelementptr inbounds i32, i32* getelementptr inbounds ([16 x i32], [16 x i32]* @b, i32 0, i32 0), i64 %idx.ext
  %1 = bitcast i32* %add.ptr3 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 32, i32 4, i1 false)
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
; CHECK-LABEL: @foo3(
; CHECK: @llvm.memcpy.p0i8.p0i8.i64
; CHECK-NOT: store
}

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 5.0.0 (https://github.com/llvm-mirror/clang.git 6cfb5bf41823be28bca09fe72dd3d4b83f4e1be8) (https://github.com/llvm-mirror/llvm.git 8708d57bbe53f61feb4630e0ac50fb938dd9a33b)"}
