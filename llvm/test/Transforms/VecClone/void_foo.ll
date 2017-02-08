; Check to make sure we can handle void foo() function

; RUN: opt -vec-clone -S < %s | FileCheck %s

; CHECK-LABEL: void @_ZGVbN4_foo()
; CHECK: entry:
; CHECK: ret void

; ModuleID = 'foo.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define void @foo() #0 {
entry:
  ret void
}

attributes #0 = { nounwind uwtable "vector-variants"="_ZGVbM4_foo1,_ZGVbN4_foo1,_ZGVcM8_foo1,_ZGVcN8_foo1,_ZGVdM8_foo1,_ZGVdN8_foo1,_ZGVeM16_foo1,_ZGVeN16_foo1" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
