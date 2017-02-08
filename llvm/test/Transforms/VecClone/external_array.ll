; Check to see that we are applying the correct updated linear index for an external array access gep.

; RUN: opt -vec-clone -S < %s | FileCheck %s

; CHECK-LABEL: @_ZGVbN4ul_foo
; CHECK: simd.loop:
; CHECK: %1 = load i32, i32* %i.addr
; CHECK: %stride.mul = mul i32 1, %index
; CHECK: %stride.add = add i32 %1, %stride.mul
; CHECK: %idxprom = sext i32 %stride.add to i64
; CHECK: %arrayidx = getelementptr inbounds [128 x i32], [128 x i32]* @ext_a, i64 0, i64 %idxprom
; CHECK: store i32 %0, i32* %arrayidx

; ModuleID = 'external_array_assign.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@ext_a = common global [128 x i32] zeroinitializer, align 16

; Function Attrs: nounwind uwtable
define void @foo(i32 %x, i32 %i) #0 {
entry:
  %x.addr = alloca i32, align 4
  %i.addr = alloca i32, align 4
  store i32 %x, i32* %x.addr, align 4
  store i32 %i, i32* %i.addr, align 4
  %0 = load i32, i32* %x.addr, align 4
  %1 = load i32, i32* %i.addr, align 4
  %idxprom = sext i32 %1 to i64
  %arrayidx = getelementptr inbounds [128 x i32], [128 x i32]* @ext_a, i64 0, i64 %idxprom
  store i32 %0, i32* %arrayidx, align 4
  ret void
}

attributes #0 = { norecurse nounwind uwtable "vector-variants"="_ZGVbM4ul_foo,_ZGVbN4ul_foo,_ZGVcM8ul_foo,_ZGVcN8ul_foo,_ZGVdM8ul_foo,_ZGVdN8ul_foo,_ZGVeM16ul_foo,_ZGVeN16ul_foo" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
