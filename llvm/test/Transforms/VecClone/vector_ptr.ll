; Test that vector of pointers are handled with correctly in loop and that incompatible function return/arg attributes are removed.

; RUN: opt -vec-clone -S < %s | FileCheck %s

; CHECK-LABEL: @_ZGVbN2v_dowork
; CHECK: simd.loop:
; CHECK: %vec.p.cast.gep = getelementptr float*, float** %vec.p.cast, i32 %index
; CHECK: %vec.p.elem = load float*, float** %vec.p.cast.gep
; CHECK: %add.ptr = getelementptr inbounds float, float* %vec.p.elem, i64 1
; CHECK: %ret.cast.gep = getelementptr float*, float** %ret.cast, i32 %index
; CHECK: store float* %add.ptr, float** %ret.cast.gep

source_filename = "vector_ptr.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: norecurse nounwind readnone uwtable
define nonnull float* @dowork(float* readnone %p) local_unnamed_addr #0 {
entry:
  %add.ptr = getelementptr inbounds float, float* %p, i64 1
  ret float* %add.ptr
}

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" "vector-variants"="_ZGVbN2v_dowork,_ZGVcN4v_dowork,_ZGVdN4v_dowork,_ZGVeN8v_
dowork,_ZGVbM2v_dowork,_ZGVcM4v_dowork,_ZGVdM4v_dowork,_ZGVeM8v_dowork" }
