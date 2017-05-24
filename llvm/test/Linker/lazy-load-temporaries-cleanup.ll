; This test ensures that metadata temporaries are properly tracked, then cleaned up

; RUN: opt -module-summary %S/Inputs/lazy-load-temporaries-cleanup.a.ll -o %t.1.o
; RUN: opt -module-summary %S/Inputs/lazy-load-temporaries-cleanup.b.ll -o %t.2.o
; RUN: llvm-link %t.1.o %t.2.o
