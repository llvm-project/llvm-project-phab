; RUN: llc -march=mips < %s | FileCheck %s

define i32 @fptoint(float %a) nounwind {
entry:
; CHECK-LABEL: trunc.w.s 
  fptosi float %a to i32		; <i32>:0 [#uses=1]
  ret i32 %0
}

define i32 @fptouint(float %a) nounwind {
entry:
; CHECK-LABEL: fptouint
; CHECK: trunc.w.s 
  fptoui float %a to i32		; <i32>:0 [#uses=1]
  ret i32 %0
}
