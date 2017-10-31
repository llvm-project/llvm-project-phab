; Check broadcast of a constant. The store of the constant should be moved inside of the loop.

; RUN: opt -vec-clone -S < %s | FileCheck %s

; CHECK-LABEL: @_ZGVbN4_foo
; CHECK: simd.loop:
; CHECK: store i32 99, i32* %ret.cast.gep

; ModuleID = 'foo.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @foo() #0 {
entry:
  ret i32 99
}

attributes #0 = { norecurse nounwind readnone uwtable "vector-variants"="_ZGVbM4_foo,_ZGVbN4_foo,_ZGVcM8_foo,_ZGVcN8_foo,_ZGVdM8_foo,_ZGVdN8_foo,_ZGVeM16_foo,_ZGVeN16_foo" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
