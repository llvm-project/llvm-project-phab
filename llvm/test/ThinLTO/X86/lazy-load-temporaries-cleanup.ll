; This test ensures that metadata temporaries are properly tracked, then cleaned up

; RUN: opt -module-summary -bitcode-mdindex-threshold=0 %S/Inputs/lazy-load-temporaries-cleanup.b.ll -o %t.2.o
; RUN: llvm-dis -materialize-metadata %t.2.o
