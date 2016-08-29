; RUN: opt -S -loop-rotate < %s -verify-loop-info | FileCheck %s
;
; Verify that LoopRotate preserves LoopSimplify form even in very peculiar loop
; structures. We manually validate the CFG with FileCheck because currently we
; can't cause a failure when LoopSimplify fails to be preserved.

define void @PR18643() {
; CHECK-LABEL: @PR18643(
entry:
  br label %outer.header
; CHECK: br label %outer.header

outer.header:
; CHECK: outer.header:
  br i1 undef, label %inner.header, label %outer.body
; CHECK-NEXT: br i1 {{[^,]*}}, label %[[INNER_PREROTATE_PREHEADER:[^,]*]], label %outer.body

; CHECK: [[INNER_PREROTATE_PREHEADER]]:
; CHECK: br label %inner.header.lr

; CHECK: inner.header.lr:
; CHECK: br i1 true, label %return, label %inner.body.lr

; CHECK: inner.body.lr:
; CHECK-NEXT: br i1 {{[^,]*}}, label %[[OUTER_LATCH_LOOPEXIT:[^,]*]], label %[[INNER_ROTATED_PREHEADER:[^,]*]]

; CHECK: [[INNER_ROTATED_PREHEADER]]:
; CHECK-NEXT: br label %inner.latch

inner.header:
; Now the latch!
; CHECK: inner.header:
  br i1 undef, label %return, label %inner.body
; CHECK-NEXT: br i1 {{[^,]*}}, label %return, label %inner.body

inner.body:
; Now the header!
; CHECK: inner.body:
  br i1 undef, label %outer.latch, label %inner.latch
; CHECK-NEXT: br i1 {{[^,]*}}, label %[[OUTER_LATCH_LOOPEXIT]], label %inner.latch

inner.latch:
; Dead!
  br label %inner.header

outer.body:
; CHECK: outer.body:
  br label %outer.latch
; CHECK-NEXT: br label %outer.latch

; CHECK: [[OUTER_LATCH_LOOPEXIT]]:
; CHECK-NEXT: br label %outer.latch

outer.latch:
; CHECK: outer.latch:
  br label %outer.header
; CHECK-NEXT: br label %outer.header

return:
; CHECK: return:
  unreachable
}
