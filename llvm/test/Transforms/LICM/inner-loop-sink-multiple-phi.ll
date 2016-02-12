; RUN: opt < %s -S -licm -loop-unswitch | FileCheck %s

define void @main() #0 {
entry:
; CHECK-LABEL: entry:
; CHECK: br label %outer.header
  br label %outer.header

outer.header:
  br label %inner.body

inner.body:
; CHECK-LABEL: inner.body:
; CHECK-NEXT: %y = phi i32 [ 0, %outer.exit ], [ %add1, %inner.body ]
; CHECK-NEXT: %z = phi i32 [ 0, %outer.exit ], [ %add2, %inner.body ]
  %y = phi i32 [ 0, %outer.header ], [ %add1, %inner.body ]
  %z = phi i32 [ 0, %outer.header ], [ %add2, %inner.body ]
  %add1 = add i32 %y, 7
  %add2 = add i32 %z, 11
; CHECK: br i1 false, label %inner.body, label %inner.body.licm_exit
  br i1 undef, label %inner.body, label %outer.inc

outer.inc:
; CHECK-LABEL: outer.inc:
; CHECK-NEXT: br i1 false, label %outer.header, label %outer.exit
  %y.lcssa = phi i32 [ %add1, %inner.body ]
  %z.lcssa = phi i32 [ %add2, %inner.body ]
  br i1 undef, label %outer.header, label %outer.exit

outer.exit:
; CHECK-LABEL: outer.exit:
; CHECK-NEXT: br label %inner.body
  %y.0.lcssa = phi i32 [ %y.lcssa, %outer.inc ]
  %z.0.lcssa = phi i32 [ %z.lcssa, %outer.inc ]
  ret void

; CHECK-LABEL: inner.body.licm_exit:
; CHECK-NEXT: %y.lcssa = phi i32 [ %add1, %inner.body ]
; CHECK-NEXT: %z.lcssa = phi i32 [ %add2, %inner.body ]
; CHECK-NEXT: ret void
}
