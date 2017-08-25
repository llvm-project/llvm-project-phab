; RUN: opt -module-summary %s -o %t1.bc
; RUN: llvm-lto2 run -dump-thin-cg-sccs %t1.bc -o %t.index.bc \
; RUN:     -r %t1.bc,external,px -r %t1.bc,l2,pl -r %t1.bc,l1,pl \
; RUN:     -r %t1.bc,simple,pl -r %t1.bc,root,pl | FileCheck %s

; CHECK: SCC (1 node) {
; CHECK-NEXT:  tmp1.bc 765152853862302398 (has loop)
; CHECK-NEXT: }

; CHECK: SCC (1 node) {
; CHECK-NEXT:  tmp1.bc 15440740835768581517
; CHECK-NEXT: }

; CHECK: SCC (1 node) {
; CHECK-NEXT:  External 5224464028922159466
; CHECK-NEXT: }

; CHECK: SCC (1 node) {
; CHECK-NEXT:  tmp1.bc 17000277804057984823 (has loop)
; CHECK-NEXT: }

; CHECK: SCC (1 node) {
; CHECK-NEXT:  tmp1.bc 5800840261926955363 (has loop)
; CHECK-NEXT: }

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare void @external()

define void @l2() {
  call void @l1()
  ret void
}

define void @l1() {
  call void @l2()
  ret void
}

define i32 @simple() {
  ret i32 23
}

define void @root() {
  call void @l1()
  call i32 @simple()
  call void @external()
  ret void
}
