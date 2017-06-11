; RUN: llvm-as %s -o %t.obj
; RUN: lld-link /entry:main /out:%t.exe %t.obj /mllvm:-time-passes 2>&1 | FileCheck %s

; RUN: lld-link /entry:main /out:%t2.exe %t.obj /msvclto /opt:lldlto=1 \ 
; RUN:          /mllvm:-time-passes 2> %t.log 
; RUN: FileCheck %s < %t.log

target datalayout = "e-m:w-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

define void @main() {
  ret void
}

; CHECK: Total Execution Time
