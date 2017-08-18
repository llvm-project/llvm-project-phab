; RUN: opt -module-summary %s -o %t1.bc
; RUN: opt -module-summary %p/Inputs/function-attr-prop-a.ll -o %t2.bc
; RUN: llvm-lto -thinlto-action=thinlink -o %t3.bc %t1.bc %t2.bc
; RUN: llvm-lto -thinlto-action=import %t1.bc -thinlto-index=%t3.bc -o - | llvm-dis -o - | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare i32 @a() readnone
define i32 @b() {
  %1 = call i32 @a()
  ret i32 %1
}

declare i32 @c() norecurse
; CHECK: define i32 @d() #0 {
define i32 @d() {
  call i32 @c()
  ret i32 1;
}
; CHECK: #0 = { norecurse }
