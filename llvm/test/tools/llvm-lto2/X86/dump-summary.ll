; RUN: opt -module-summary %s -o %t.o
; RUN: llvm-lto2 dump-summary %t.o | egrep 'Kind:\s+GlobalVar'
; RUN: llvm-lto2 dump-summary %t.o | egrep 'Kind:\s+Function'
; RUN: llvm-lto2 dump-summary %t.o | egrep 'Kind:\s+Alias'

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"
@G = constant i32 2048, align 4
@a = weak alias i32, i32* @G

define void @boop() {
  tail call void @afun()
  ret void
}

declare void @afun()
declare void @testtest()
