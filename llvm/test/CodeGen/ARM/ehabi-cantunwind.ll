; Check that ARM EHABI EXIDX_CANTUNWIND directive is not generated when requested.
;
; - If the function does not throw (nounwind attribute), do not generate unwind table. 
; - If the function doesn not throw but unwind table are required (nounwind uwtables
;   attributes), do not generate EXIDX_CANTUNWIND value.

declare void @bar()

; RUN: llc -mtriple armv7-unknown--eabi -filetype=asm  \
; RUN:     -disable-arm-cantunwind -o - %s \
; RUN:   | FileCheck %s --check-prefix=CHECK-NOTHROW

define void @test0 () #0 {
entry:
  call void @bar()
  ret void
}

; CHECK-NOTHROW-LABEL: test0:
; CHECK-NOTHROW-NOT:  .fnstart
; CHECK-NOTHROW-NOT:  .cantunwind
; CHECK-NOTHROW-NOT:  .fnend

define void @test1 () #1 {
entry:
  call void @bar()
  ret void
}

; CHECK-NOTHROW-LABEL: test1:
; CHECK-NOTHROW:       .fnstart
; CHECK-NOTHROW-NOT:   .cantunwind
; CHECK-NOTHROW:       .fnend

attributes #0 = { nounwind }
attributes #1 = { nounwind uwtable }
