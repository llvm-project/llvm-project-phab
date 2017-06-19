; This test ensures that metadata temporaries are properly tracked, then cleaned up

; RUN: opt -module-summary -bitcode-mdindex-threshold=0 %s | llvm-dis -materialize-metadata -set-importing -o - | FileCheck %s
; CHECK: !named = !{!0}

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define void  @patatino() {
  ret void
}

!named = !{!0}

!0  =  distinct !{!1, i8 0}
!1  = !{i8 1}
