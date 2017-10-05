; RUN: opt < %s -partial-inliner -S | FileCheck %s
; RUN: opt < %s -passes=partial-inliner -S | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; For this test case the partial inliner tries to inline the call, but then
; aborts. This test case verifies that the abort succeeds without crash.

define void @quantum_bmeasure(i32 %pos) {
entry:
; CHECK: tail call void (i8, ...) @quantum_objcode_put(i8 zeroext undef, i32 %pos)
  tail call void (i8, ...) @quantum_objcode_put(i8 zeroext undef, i32 %pos)
  unreachable
}

define void @quantum_objcode_put(i8 zeroext %operation, ...) local_unnamed_addr {
entry:
  br i1 undef, label %cleanup, label %if.end

if.end:                                           ; preds = %entry
  unreachable

cleanup:                                          ; preds = %entry
  ret void
}

!llvm.module.flags = !{!0}

!0 = !{i32 1, !"wchar_size", i32 4}
