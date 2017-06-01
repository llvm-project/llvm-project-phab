; Test to check for correct value numbering when there are multiple copies of a
; (linkonce) function.
; RUN: opt -module-summary %s -o %t.o
; RUN: opt -module-summary %p/Inputs/thinlto-function-summary-callgraph-linkonce.ll -o %t2.o
; RUN: llvm-lto -thinlto -o %t3 %t.o %t2.o
; RUN: llvm-bcanalyzer -dump %t3.thinlto.bc | FileCheck %s --check-prefix=COMBINED

; COMBINED:       <GLOBALVAL_SUMMARY_BLOCK
; COMBINED-NEXT:    <VERSION
; func:
; COMBINED-NEXT:    <VALUE_GUID op0=1 op1=7289175272376759421/>
; main:
; COMBINED-NEXT:    <VALUE_GUID op0=2
; Two summaries for func:
; COMBINED-NEXT:    <COMBINED {{.*}} op0=1
; COMBINED-NEXT:    <COMBINED {{.*}} op0=1
; main should have an edge to func:
; COMBINED-NEXT:    <COMBINED {{.*}} op5=1/>
; COMBINED-NEXT:  </GLOBALVAL_SUMMARY_BLOCK>

; ModuleID = 'thinlto-function-summary-callgraph-linkonce.ll'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define linkonce_odr void @func() #0 {
entry:
      ret void
}

; Function Attrs: nounwind uwtable
define void @main() #0 {
entry:
    call void () @func()
    ret void
}
