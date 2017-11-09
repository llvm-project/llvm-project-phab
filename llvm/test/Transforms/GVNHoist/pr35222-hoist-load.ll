; RUN: opt -S -gvn-hoist < %s | FileCheck %s
; CHECK-LABEL: @build_tree
; CHECK: load
; CHECK: load
; Check that the load is not hoisted because the call can potentially
; modify the global

target datalayout = "e-m:e-p:32:32-i64:64-v128:64:128-a:0:32-n32-S64"

@heap = external global i32, align 4

define i32 @build_tree() unnamed_addr {
entry:
  br label %do.body

do.body:                                          ; preds = %do.body, %entry
  %tmp9 = load i32, i32* @heap, align 4
  %cmp = call i1 @pqdownheap(i32 %tmp9)
  br i1 %cmp, label %do.body, label %do.end

do.end:                                           ; preds = %do.body
  %tmp20 = load i32, i32* @heap, align 4
  ret i32 %tmp20
}

; Check that nothing is hoisted here:
; CHECK-LABEL: @build_irred
; CHECK: load
; CHECK: load
; CHECK: load
; CHECK: load
define i32 @build_irred() unnamed_addr {
entry:
  %tmp8 = load i32, i32* @heap, align 4
  %cmp0 = call i1 @pqdownheap(i32 %tmp8)
  br i1 %cmp0, label %do.body, label %do.body1

do.body:
  %tmp9 = load i32, i32* @heap, align 4
  br label %do.body1

do.body1:
  %tmp10 = load i32, i32* @heap, align 4
  %cmp2 = call i1 @pqdownheap(i32 %tmp10)
  br i1 %cmp2, label %do.body, label %do.end

do.end:
  %tmp20 = load i32, i32* @heap, align 4
  ret i32 %tmp20
}



declare i1 @pqdownheap(i32)
