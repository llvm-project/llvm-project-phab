; RUN: not llc %s -o - 2>&1 | FileCheck %s

; ModuleID = 't-call-float-arg.c'
source_filename = "t-call-float-arg.c"
target datalayout = "e-m:e-p:32:32-i64:64-v128:64:128-a:0:32-n32-S64"
target triple = "thumbv6--linux-gnueabihf"

; Function Attrs: noinline nounwind optnone
define void @g() #0 {
; CHECK: LLVM ERROR: The aapcs_vfp calling convention is not supported on this target.
entry:
  call arm_aapcs_vfpcc void @f(float 1.000000e+00)
  ret void
}

declare arm_aapcs_vfpcc void @f(float) #1

attributes #0 = { noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="arm1136j-s" "target-features"="+dsp,+strict-align,-crypto,-d16,-fp-armv8,-fp-only-sp,-fp16,-neon,-vfp2,-vfp3,-vfp4" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="arm1136j-s" "target-features"="+dsp,+strict-align,-crypto,-d16,-fp-armv8,-fp-only-sp,-fp16,-neon,-vfp2,-vfp3,-vfp4" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"min_enum_size", i32 4}
!2 = !{!"clang version 5.0.0 "}
