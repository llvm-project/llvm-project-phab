; RUN: opt -mergesimilarfunc -mergesimilarfunc-level=all -mergesimilarfunc-min-insts=0 -mergesimilarfunc-diff-min-insts=0 < %s -S | FileCheck %s

; This test checks if we can tell the difference between the two corresponding
; calls to foobar.  Each call has a  reference to the containing function as the
; only argument to foobar.
; If we cannot tell the difference between these two calls, then the functions
; foo and bar below will be considered identical and bar will simply be patched
; up to call foo. However, these functions are not identical as the calls to foobar
; are themselves different. So, these functions are similar, but not identical.
; bar should be patched up to call foo__merged which has code to deal with the
; two different calls to foobar.

; CHECK: define i32 @bar(i32 %a) #0 {
; CHECK: %1 = tail call i32 @foo__merged(i32 %a, i32 1)
define i32 @foo(i32 %a) nounwind {
entry:
  %a.addr = alloca i32, align 4
  %b = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  %call = call i32 @foobar(i32 (i32)* @foo)
  store i32 %call, i32* %b, align 4
  %0 = load i32, i32* %a.addr, align 4
  %1 = load i32, i32* %b, align 4
  %add = add nsw i32 %0, %1
  ret i32 %add
}

declare i32 @foobar(i32 (i32)*)

define i32 @bar(i32 %a) nounwind {
entry:
  %a.addr = alloca i32, align 4
  %b = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  %call = call i32 @foobar(i32 (i32)* @bar)
  store i32 %call, i32* %b, align 4
  %0 = load i32, i32* %a.addr, align 4
  %1 = load i32, i32* %b, align 4
  %add = add nsw i32 %0, %1
  ret i32 %add
}
