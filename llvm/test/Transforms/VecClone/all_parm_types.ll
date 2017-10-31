; Test all different kinds of parameters (uniform, linear, vector), multiple uses of linear k, and that stride calculations can handle type conversions.

; RUN: opt -vec-clone -S < %s | FileCheck %s

; CHECK-LABEL: @_ZGVbN4uvl_dowork
; CHECK: simd.loop:
; CHECK: %stride.mul{{.*}} = mul i32 1, %index
; CHECK: %stride.cast{{.*}} = sext i32 %stride.mul{{.*}}
; CHECK: %stride.add{{.*}} = add i64 %k, %stride.cast{{.*}}
; CHECK: %arrayidx = getelementptr inbounds float, float* %a, i64 %stride.add{{.*}}
; CHECK: %stride.mul{{.*}} = mul i32 1, %index
; CHECK: %stride.cast{{.*}} = bitcast i32 %stride.mul{{.*}} to float
; CHECK: %stride.add{{.*}} = fadd float %conv, %stride.cast{{.*}}
; CHECK: %add{{.*}} = fadd float %add, %stride.add{{.*}}

; ModuleID = 'rfc.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define float @dowork(float* %a, float %b, i64 %k) #0 {
entry:
  %arrayidx = getelementptr inbounds float, float* %a, i64 %k
  %0 = load float, float* %arrayidx, align 4, !tbaa !2
  %call = call float @sinf(float %0) #5
  %add = fadd float %call, %b
  %conv = sitofp i64 %k to float
  %add1 = fadd float %add, %conv
  ret float %add1
}

; Function Attrs: nounwind
declare float @sinf(float) #1

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" "vector-variants"="_ZGVbN4uvl_dowork,_ZGVcN8uvl_dowork,_ZGVdN8uvl_dowork,_ZGVeN16uvl_dowork,_ZGVbM4uvl_dowork,_ZGVcM8uvl_dowork,_ZGVdM8uvl_dowork,_ZGVeM16uvl_dowork" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0 (trunk 316400)"}
!2 = !{!3, !3, i64 0}
!3 = !{!"float", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
