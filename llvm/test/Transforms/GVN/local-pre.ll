; RUN: opt < %s -gvn -S | FileCheck %s

define i32 @test1(i32 %p, i32 %q) {
; CHECK-LABEL: @test1
block1:
    %cmp = icmp eq i32 %p, %q 
	br i1 %cmp, label %block2, label %block3

block2:
 %a = add i32 %p, 1
 br label %block4

; CHECK: block3:
; CHECK-NEXT: add i32 %p, 1
block3:
  br label %block4

block4:
; CHECK: block4:
; CHECK-NEXT: phi
; CHECK-NEXT: ret
  %b = add i32 %p, 1
  ret i32 %b
}

define i32 @test2(i32 %p, i32 %q) {
; CHECK-LABEL: @test2
; CHECK: block1:
block1:
    %cmp = icmp eq i32 %p, %q
        br i1 %cmp, label %block2, label %block3

block2:
 %a = sdiv i32 %p, %q
 br label %block4

block3:
  br label %block4

; CHECK: block4
; CHECK-NEXT: call
; CHECK-NEXT: %b = sdiv
; CHECK-NEXT: ret i32 %b

block4:
  call void @may_exit() nounwind
  %b = sdiv i32 %p, %q
  ret i32 %b
}

declare void @may_exit() nounwind
