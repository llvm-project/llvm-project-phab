; RUN: opt -module-summary %s -o %s.o
; RUN: llvm-lto2 dump-summary %s.o | FileCheck %s
; CHECK: ModuleSummaryIndex:
; CHECK:   {{.*}}:
; CHECK:     # X
; CHECK:     - Kind:            GlobalVar
; CHECK:       Linkage:         ExternalLinkage
; CHECK:       NotEligibleToImport: false
; CHECK:       Live:            false
; CHECK:   {{.*}}:
; CHECK:     # t
; CHECK:     - Kind:            Function
; CHECK:       Linkage:         ExternalLinkage
; CHECK:       NotEligibleToImport: false
; CHECK:       Live:            false
; CHECK:       InstCount:       2
; CHECK:       Calls:
; CHECK:         - GUID:        {{.*}} # b
; CHECK:           Hotness:         Unknown
; CHECK:   {{.*}}:
; CHECK:     # a
; CHECK:     - Kind:            Alias
; CHECK:       Linkage:         WeakAnyLinkage
; CHECK:       NotEligibleToImport: false
; CHECK:       Live:            false
; CHECK:       Aliasee:         {{.*}} # X
; CHECK:   {{.*}}:
; CHECK:     # f
; CHECK:     - Kind:            Function
; CHECK:       Linkage:         ExternalLinkage
; CHECK:       NotEligibleToImport: false
; CHECK:       Live:            false
; CHECK:       Refs:
; CHECK:         - GUID:        {{.*}} # a
; CHECK:       InstCount:       2


target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@X = constant i32 42, section "foo", align 4
@a = weak alias i32, i32* @X

define void  @f() {
  %1 = load i32, i32* @a
  ret void
}

define void @t() {
  tail call void @b()
  ret void
}

declare void @b()
