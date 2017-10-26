; RUN: llc -verify-machineinstrs -mtriple=powerpc64le-unknown-linux-gnu -mattr=+save-toc-indirect < %s | FileCheck %s --check-prefix=TOC_INDIRECT
; RUN: llc -verify-machineinstrs -mtriple=powerpc64le-unknown-linux-gnu < %s | FileCheck %s

define signext i32 @test(i32 signext %i, i32 (i32)* nocapture %Func, i32 (i32)* nocapture %Func2) {
; TOC_INDIRECT-LABEL: test:
; TOC_INDIRECT:       # BB#0: # %entry
; TOC_INDIRECT:    std 2, 24(1)
; TOC_INDIRECT:    bctrl
; TOC_INDIRECT:    ld 2, 24(1)
; TOC_INDIRECT-NOT:    std 2, 24(1)
; TOC_INDIRECT:    bctrl
; TOC_INDIRECT:    ld 2, 24(1)

; CHECK-LABEL: test:
; CHECK:       # BB#0: # %entry
; CHECK:    std 2, 24(1)
; CHECK:    bctrl
; CHECK:    ld 2, 24(1)
; CHECK:    std 2, 24(1)
; CHECK:    bctrl
; CHECK:    ld 2, 24(1)

entry:
  %call = tail call signext i32 %Func(i32 signext %i)
  %call1 = tail call signext i32 %Func2(i32 signext %i)
  %add2 = add nsw i32 %call1, %call
  ret i32 %add2
}
