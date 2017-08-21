; RUN: opt -module-summary %s -o %t.o
; RUN: llvm-bcanalyzer -dump %t.o | FileCheck %s

; CHECK: <GLOBALVAL_SUMMARY_BLOCK
; ensure @h is marked norecurse
; CHECK:  <PERMODULE {{.*}} op0=0 {{.*}} op3=5
; ensure @i is marked returndoesnotalias
; CHECK:  <PERMODULE {{.*}} op0=1 {{.*}} op3=9

define void @h() norecurse {
   ret void
}

define noalias i8* @i() {
   %r = alloca i8
   ret i8* %r
}
