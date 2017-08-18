
;
; RUN: llc -march=sparc < %s 2>&1 | FileCheck %s
; RUN: llc -march=sparcv9 < %s 2>&1 | FileCheck --check-prefix=CHECK --check-prefix=CHECK64 %s

; check that fitod used for SPISD::ITOF
;
; CHECK-LABEL:  Primitive_us 
; CHECK:fitod

; Function Attrs: norecurse nounwind readnone
define double @Primitive_us(i16 zeroext) local_unnamed_addr #0 {
  %2 = uitofp i16 %0 to double
  ret double %2
}

; check that fxtod used for SPISD::XTOF
;
; CHECK-LABEL:  Primitive_ui
; CHECK64:fxtod  

; Function Attrs: norecurse nounwind readnone
define double @Primitive_ui(i32 zeroext) local_unnamed_addr #0 {
  %2 = uitofp i32 %0 to double
  ret double %2
}


