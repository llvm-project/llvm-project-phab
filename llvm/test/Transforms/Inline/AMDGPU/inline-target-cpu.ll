; RUN: opt -mtriple=amdgcn-amd-amdhsa -S -inline < %s | FileCheck %s
; RUN: opt -mtriple=amdgcn-amd-amdhsa -S -passes='cgscc(inline)' < %s | FileCheck %s

; CHECK-LABEL: @func_no_target_cpu(
define i32 @func_no_target_cpu() #0 {
  ret i32 0
}

; CHECK-LABEL: @target_cpu_call_no_target_cpu(
; CHECK-NEXT: ret i32 0
define i32 @target_cpu_call_no_target_cpu() #1 {
  %call = call i32 @func_no_target_cpu()
  ret i32 %call
}

; CHECK-LABEL: @target_cpu_target_features_call_no_target_cpu(
; CHECK-NEXT: ret i32 0
define i32 @target_cpu_target_features_call_no_target_cpu() #2 {
  %call = call i32 @func_no_target_cpu()
  ret i32 %call
}

; CHECK-LABEL: @fp32_denormals(
define i32 @fp32_denormals() #3 {
  ret i32 0
}

; CHECK-LABEL: @no_fp32_denormals_call_f32_denormals(
; CHECK-NEXT: call i32 @fp32_denormals()
define i32 @no_fp32_denormals_call_f32_denormals() #4 {
  %call = call i32 @fp32_denormals()
  ret i32 %call
}

; Make sure gfx9 can call unspecified functions because of movrel
; feature change.
; CHECK-LABEL: @gfx9_target_features_call_no_target_cpu(
; CHECK-NEXT: ret i32 0
define i32 @gfx9_target_features_call_no_target_cpu() #5 {
  %call = call i32 @func_no_target_cpu()
  ret i32 %call
}

attributes #0 = { nounwind }
attributes #1 = { nounwind "target-cpu"="fiji" }
attributes #2 = { nounwind "target-cpu"="fiji" "target-features"="+fp32-denormals" }
attributes #3 = { nounwind "target-features"="+fp32-denormals" }
attributes #4 = { nounwind "target-features"="-fp32-denormals" }
attributes #5 = { nounwind "target-cpu"="gfx900" }
