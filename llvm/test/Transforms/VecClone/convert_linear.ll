; Check handling of upconverting a linear (variable %i) to ensure stride calculation
; is inserted correctly and the old convert (sext) uses the stride instead of the old
; reference to %i.

; RUN: opt -vec-clone -S < %s | FileCheck %s

; CHECK-LABEL: @_ZGVbN2vl_foo
; CHECK: simd.loop:
; CHECK: %0 = load i32, i32* %i.addr
; CHECK-NEXT: %stride.mul = mul i32 1, %index
; CHECK-NEXT: %stride.add = add i32 %0, %stride.mul
; CHECK-NEXT: %conv = sext i32 %stride.add to i64

; ModuleID = 'convert.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define i64 @foo(i64 %x, i32 %i) #0 {
entry:
  %x.addr = alloca i64, align 8
  %i.addr = alloca i32, align 4
  store i64 %x, i64* %x.addr, align 8
  store i32 %i, i32* %i.addr, align 4
  %0 = load i32, i32* %i.addr, align 4
  %conv = sext i32 %0 to i64
  %1 = load i64, i64* %x.addr, align 8
  %add = add nsw i64 %conv, %1
  ret i64 %add
}

attributes #0 = { norecurse nounwind readnone uwtable "vector-variants"="_ZGVbM2vl_foo,_ZGVbN2vl_foo,_ZGVcM4vl_foo,_ZGVcN4vl_foo,_ZGVdM4vl_foo,_ZGVdN4vl_foo,_ZGVeM8vl_foo,_ZGVeN8vl_foo" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
