; RUN: opt -S -basicaa -functionattrs-td < %s | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: define i32 @bar()

; Function Attrs: nounwind uwtable
define i32 @bar() #0 {
entry:
  %call = tail call noalias i8* @malloc(i64 12345) #3
  %0 = bitcast i8* %call to i32*
  store i32 10, i32* %0, align 4, !tbaa !0
  tail call fastcc void @foo(i32* %0)
  ret i32 0
}

; Function Attrs: nounwind
declare noalias i8* @malloc(i64) #1

; CHECK: define internal fastcc void @foo(i32* noalias dereferenceable(12345) %x)

; Function Attrs: noinline nounwind uwtable
define internal fastcc void @foo(i32* dereferenceable(12345) %x) #2 {
entry:
  %arrayidx = getelementptr inbounds i32* %x, i64 5
  store i32 10, i32* %arrayidx, align 4, !tbaa !0
  tail call void @goo(i32* %x) #3
  ret void
}

declare void @goo(i32*) #3

attributes #0 = { nounwind uwtable }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind uwtable }
attributes #3 = { nounwind }

!0 = metadata !{metadata !1, metadata !1, i64 0}
!1 = metadata !{metadata !"int", metadata !2, i64 0}
!2 = metadata !{metadata !"omnipotent char", metadata !3, i64 0}
!3 = metadata !{metadata !"Simple C/C++ TBAA"}

