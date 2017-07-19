; Check the linkage types in both the per-module and combined summaries.
; RUN: opt -module-summary %s -o %t.o
; RUN: llvm-bcanalyzer -dump %t.o | FileCheck %s
; RUN: llvm-lto -thinlto -o %t2 %t.o
; RUN: llvm-bcanalyzer -dump %t2.thinlto.bc | FileCheck %s --check-prefix=COMBINED

; Flags should be 0x37 (55) for local linkage (0x7), not being importable
; (0x10) due to local linkage plus having a section, and a live root (0x20)
; due to having a section.
; CHECK: <PERMODULE {{.*}} op1=55
; COMBINED-DAG: <COMBINED {{.*}} op2=55
define internal void @functionWithSection() section "some_section" {
    ret void
}
