; RUN: opt < %s -postdomtree -analyze | FileCheck %s
define internal void @f() {
entry:
  br i1 undef, label %bb35, label %bb3.i

bb3.i:
  br label %bb3.i

bb35.loopexit3:
  br label %bb35

bb35:
  ret void
}
;CHECK:Inorder PostDominator Tree:
;CHECK-NEXT:  [1]  <<exit node>>
;CHECK-NEXT:    [2] %bb35
;CHECK-NEXT:      [3] %entry
;CHECK-NEXT:      [3] %bb35.loopexit3
;CHECK-NEXT:    [2] %bb3.i
