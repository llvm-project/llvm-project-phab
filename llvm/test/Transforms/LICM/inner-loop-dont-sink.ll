; RUN: opt < %s -S -licm | FileCheck %s

define void @main() #0 {
entry:
; CHECK-LABEL: entry:
; CHECK-NEXT: br label %outer.header
  br label %outer.header

outer.header:
; CHECK-LABEL: outer.header:
; CHECK: br i1 undef, label %if.then, label %outer.latch
  br i1 undef, label %if.then, label %outer.latch

if.then:
; CHECK-LABEL: if.then:
  %call = tail call i32 @generate()
; CHECK: br label %inner.body
  br label %inner.body

inner.body:
; CHECK-LABEL: inner.body:
  %arrayctor.done = icmp eq i32 undef, %call
; CHECK: br i1 %arrayctor.done, label %outer.latch.loopexit, label %inner.body
  br i1 %arrayctor.done, label %outer.latch, label %inner.body

outer.latch:
  br i1 undef, label %outer.exit, label %outer.header

outer.exit:
  ret void
}

declare i32 @generate() #1
