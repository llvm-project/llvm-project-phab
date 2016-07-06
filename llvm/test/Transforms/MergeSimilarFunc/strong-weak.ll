; RUN: opt -mergesimilarfunc -mergesimilarfunc-level=all -mergesimilarfunc-min-insts=0 -mergesimilarfunc-diff-min-insts=1 < %s -S
; REQUIRES: asserts
; CHECK-NOT: __merged
define weak i32 @bar(i32 %a) nounwind {
entry:
  %a.addr = alloca i32, align 4
  %b = alloca i32, align 4
  %d = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  %0 = load i32, i32* %a.addr, align 4
  %1 = load i32, i32* %d, align 4
  %add = add nsw i32 %0, %1
  %call = call i32 bitcast (i32 (...)* @foo to i32 ()*)()
  %add1 = add nsw i32 %add, %call
  store i32 %add1, i32* %b, align 4
  %2 = load i32, i32* %b, align 4
  ret i32 %2
}

declare i32 @foo(...)

define i32 @barred(i32 %a) nounwind {
entry:
  %a.addr = alloca i32, align 4
  %b = alloca i32, align 4
  %d = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  %0 = load i32, i32* %a.addr, align 4
  %1 = load i32, i32* %d, align 4
  %add = add nsw i32 %0, %1
  %call = call i32 bitcast (i32 (...)* @boohoo to i32 ()*)()
  %add1 = add nsw i32 %add, %call
  store i32 %add1, i32* %b, align 4
  %2 = load i32, i32* %b, align 4
  ret i32 %2
}

declare i32 @boohoo(...)
