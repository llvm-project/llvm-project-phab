
; RUN: opt -analyze -scalar-evolution < %s | FileCheck %s --check-prefix=CHECK-ANALYSIS

define i32 @foo() {
  br label %do_start

do_start:
  %inc = phi i32 [ 0, %0 ], [ %add, %do_start ]
  %cmp = icmp slt i32 %inc, 10000
  %add_cond = add nsw i32 %inc, 2
  %sel = select i1 %cmp, i32 %add_cond, i32 %inc
  %add = add nsw i32 %sel, 1
  br i1 %cmp, label %do_start, label %do_end

; CHECK-ANALYSIS: Loop %do_start: backedge-taken count is 3334
; CHECK-ANALYSIS: Loop %do_start: max backedge-taken count is 3334
; CHECK-ANALYSIS: Loop %do_start: Predicated backedge-taken count is 3334

do_end:
  ret i32 0
}