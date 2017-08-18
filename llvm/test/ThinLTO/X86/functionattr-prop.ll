; RUN: llvm-as %s -o %t1.o
; RUN: llvm-as %p/Inputs/function-attr-prop-a.ll -o %t2.o
; RUN: llvm-lto2 run -o %t3.o %t2.o %t1.o -r %t2.o,c,px -r %t1.o,d,px -r %t1.o,c,l -save-temps
; RUN: llvm-dis < %t3.o.0.4.opt.bc -o - | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare i32 @c()
; CHECK: define i32 @d() {{.*}} #0
define i32 @d() {
  call i32 @c()
  ret i32 1;
}
; CHECK: #0 = { norecurse
