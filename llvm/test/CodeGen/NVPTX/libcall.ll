; RUN: llc < %s -march=nvptx | FileCheck %s
; Allow to make libcalls that are defined in current module

; An intrinsic
declare double @llvm.powi.f64(double, i32) nounwind readnone

; Underlying libcall
define double @__powidf2(double, i32) {
  ret double 0.0
}

define double @powi() {
  ; CHECK:      { // callseq 0, 0
  ; CHECK:      call.uni (retval0),
  ; CHECK-NEXT: __powidf2,
  ; CHECK-NEXT: (
  ; CHECK-NEXT: param0,
  ; CHECK-NEXT: param1
  ; CHECK-NEXT: );
  ; CHECK-NEXT: ld.param.f64 %fd{{[0-9]+}}, [retval0+0];
  ; CHECK-NEXT: } // callseq 0
  %1 = call double @llvm.powi.f64(double 1.0, i32 undef)
  ret double %1
}
