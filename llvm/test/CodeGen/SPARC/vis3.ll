
;
; RUN: llc -march=sparc -mcpu=niagara4 < %s 2>&1 | FileCheck %s
; RUN: llc -march=sparcv9 -mcpu=niagara4 < %s 2>&1 | FileCheck --check-prefix=CHECK --check-prefix=CHECK64 %s

; check that movwtos used for bitcast
;
; CHECK-LABEL:  Primitive_us
; CHECK:movwtos

; Function Attrs: norecurse nounwind readnone
define double @Primitive_us(i16 zeroext) local_unnamed_addr #0 {
  %2 = uitofp i16 %0 to double
  ret double %2
}

; check for movxtod used for bitcast 
;
; CHECK-LABEL:  Primitive_ui
; CHECK64:movxtod  

; Function Attrs: norecurse nounwind readnone
define double @Primitive_ui(i32 zeroext) local_unnamed_addr #0 {
  %2 = uitofp i32 %0 to double
  ret double %2
}


