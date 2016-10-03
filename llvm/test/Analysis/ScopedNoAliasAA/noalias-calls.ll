; RUN: opt < %s -basicaa -scoped-noalias -aa-eval -evaluate-aa-metadata -print-all-alias-modref-info -disable-output 2>&1 | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #1

; Function Attrs: nounwind
declare void @hey() #1

; Function Attrs: nounwind uwtable
define void @foo(i8* nocapture %a, i8* nocapture readonly %c, i8* nocapture %b) #2 {
entry:
  %l.i = alloca i8, i32 512, align 1
  %0 = call i8* @llvm.noalias.p0i8(i8* %a, metadata !0) #0
  %1 = call i8* @llvm.noalias.p0i8(i8* %c, metadata !3) #0
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %b, i64 16, i32 16, i1 false) #1, !noalias !5
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %b, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
  call void @hey() #1, !noalias !5
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %l.i, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
  ret void
}

; CHECK-LABEL: Function: foo:
; CHECK: Just Ref:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %b, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %b, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: Just Mod:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %b, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: Just Ref:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %b, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @hey() #1, !noalias !5
; CHECK: NoModRef:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %b, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %l.i, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: Just Mod:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %b, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %b, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: NoModRef:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %b, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: Just Mod:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %b, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @hey() #1, !noalias !5
; CHECK: NoModRef:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %b, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %l.i, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: Just Mod:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %b, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: NoModRef:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %b, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: NoModRef:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @hey() #1, !noalias !5
; CHECK: NoModRef:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %l.i, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: Just Mod:   call void @hey() #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %b, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: Both ModRef:   call void @hey() #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %b, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: NoModRef:   call void @hey() #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: NoModRef:   call void @hey() #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %l.i, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: NoModRef:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %l.i, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %b, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: NoModRef:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %l.i, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %b, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: NoModRef:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %l.i, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %0, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5
; CHECK: NoModRef:   call void @llvm.memcpy.p0i8.p0i8.i64(i8* %l.i, i8* %1, i64 16, i32 16, i1 false) #1, !noalias !5 <->   call void @hey() #1, !noalias !5

; Function Attrs: nounwind
declare i8* @llvm.noalias.p0i8(i8*, metadata) #0

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }
attributes #2 = { nounwind uwtable }


!0 = !{!1}
!1 = distinct !{!1, !2, !"hello: %a"}
!2 = distinct !{!2, !"hello"}
!3 = !{!4}
!4 = distinct !{!4, !2, !"hello: %c"}
!5 = !{!1, !4}
