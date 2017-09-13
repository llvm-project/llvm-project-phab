; RUN: opt -thinlto-bc -o %t %s
; RUN: llvm-modextract -b -n 0 -o %t0 %t
; RUN: llvm-modextract -b -n 1 -o %t1 %t
; RUN: llvm-dis -o - %t0 | FileCheck --check-prefix=M0 %s
; RUN: llvm-dis -o - %t1 | FileCheck --check-prefix=M1 %s

; M0-NOT: @"g2$
; M1: @g2 = internal
@g1 = global i8* bitcast (i8** @g2 to i8*), !type !0
@g2 = internal global i8* null, !type !0

!0 = !{i32 0, !"typeid"}
