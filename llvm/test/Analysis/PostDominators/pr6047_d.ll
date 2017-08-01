; RUN: opt < %s -postdomtree -analyze | FileCheck %s
define internal void @f() {
entry:
  br i1 1, label %a, label %b

a:
br label %c

b:
br label %c

c:
  br i1 undef, label %bb35, label %bb3.i

bb3.i:
  br label %bb3.i

bb35.loopexit3:
  br label %bb35

bb35:
  ret void
}
; CHECK: [4] %entry

; CHECK: Inorder PostDominator Tree
; CHECK-NEXT:   [1]  <<exit node>>
; CHECK-NEXT:     [2] %bb35
; CHECK-NEXT:       [3] %c
; CHECK-NEXT:         [4] %a
; CHECK-NEXT:         [4] %entry
; CHECK-NEXT:         [4] %b
; CHECK-NEXT:       [3] %bb35.loopexit3
; CHECK-NEXT:     [2] %bb3.i
; CHECK-NEXT: Roots: %bb35 %bb3.i
