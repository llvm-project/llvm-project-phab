; RUN: opt -module-summary %s -o %t.o
; RUN: llvm-bcanalyzer -dump %t.o | FileCheck %s

; CHECK: <GLOBALVAL_SUMMARY_BLOCK
; ensure @f is marked readnone
; CHECK:  <PERMODULE {{.*}} op0=0 {{.*}} op3=1
; ensure @g is marked readonly
; CHECK:  <PERMODULE {{.*}} op0=1 {{.*}} op3=2
; ensure @h is marked norecurse
; CHECK:  <PERMODULE {{.*}} op0=2 {{.*}} op3=4
; ensure @i is marked returndoesnotalias
; CHECK:  <PERMODULE {{.*}} op0=4 {{.*}} op3=8

define void @f() readnone {
   ret void
}
define void @g() readonly {
   ret void
}
define void @h() norecurse {
   ret void
}

declare noalias i8* @malloc(i32)

define noalias i8* @i() {
   %r = call noalias i8* @malloc(i32 1)
   ret i8* %r
}
