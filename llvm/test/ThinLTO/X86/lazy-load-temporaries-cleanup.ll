; This test ensures that metadata temporaries are properly tracked, then cleaned up

; RUN: opt -module-summary -bitcode-mdindex-threshold=0 %S/Inputs/lazy-load-temporaries-cleanup.b.ll | llvm-dis -materialize-metadata -set-importing
