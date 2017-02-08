; Check to see that the linear parameter i is updated with the correct stride, indicated by a mul/add instruction sequence after the load.

; RUN: opt -vec-clone -S < %s | FileCheck %s

; CHECK-LABEL: @_ZGVbN4lu_foo
; CHECK: simd.loop:
; CHECK: %1 = load i32, i32* %i.addr
; CHECK: %stride.mul = mul i32 1, %index
; CHECK: %stride.add = add i32 %1, %stride.mul
; CHECK: %add = add nsw i32 %0, %stride.add

; ModuleID = 'linear.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @foo(i32 %i, i32 %x) #0 {
entry:
  %i.addr = alloca i32, align 4
  %x.addr = alloca i32, align 4
  store i32 %i, i32* %i.addr, align 4
  store i32 %x, i32* %x.addr, align 4
  %0 = load i32, i32* %x.addr, align 4
  %1 = load i32, i32* %i.addr, align 4
  %add = add nsw i32 %0, %1
  ret i32 %add
}

attributes #0 = { norecurse nounwind readnone uwtable "vector-variants"="_ZGVbM4lu_foo,_ZGVbN4lu_foo,_ZGVcM8lu_foo,_ZGVcN8lu_foo,_ZGVdM8lu_foo,_ZGVdN8lu_foo,_ZGVeM16lu_foo,_ZGVeN16lu_foo" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
