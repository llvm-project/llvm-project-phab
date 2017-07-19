; Test to ensure we don't internalize or treat as dead a global value
; with a section. Otherwise, ELF linker generation of __start_"sectionname"
; and __stop_"sectionname" symbols would no occur and we can end up
; with undefined references at link time.

; Do setup work for all below tests: generate bitcode
; RUN: opt -module-summary %s -o %t.bc
; RUN: opt -module-summary %p/Inputs/global_with_section.ll -o %t2.bc

; Check results of internalization

; RUN: llvm-lto -thinlto-action=thinlink -o %t3.bc %t.bc %t2.bc
; RUN: llvm-lto -thinlto-action=internalize -exported-symbol=foo %t.bc -thinlto-index=%t3.bc -o - | llvm-dis -o - | FileCheck %s
; RUN: llvm-lto -thinlto-action=internalize -exported-symbol=foo %t2.bc -thinlto-index=%t3.bc -o - | llvm-dis -o - | FileCheck %s --check-prefix=CHECK2

; RUN: llvm-lto2 run %t.bc %t2.bc -o %t.o -save-temps \
; RUN:     -r=%t.bc,foo,pxl \
; RUN:     -r=%t.bc,var_with_section,pl \
; RUN:     -r=%t.bc,deadfunc,pl \
; RUN:     -r=%t.bc,deadfunc2,pl \
; RUN:     -r=%t2.bc,deadfunc2,pl
; RUN: llvm-dis < %t.o.0.2.internalize.bc | FileCheck %s
; RUN: llvm-dis < %t.o.1.2.internalize.bc | FileCheck %s --check-prefix=CHECK2

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; We should not internalize @var_with_section due to section
; CHECK: @var_with_section = global i32 0, section "some_section"
@var_with_section = global i32 0, section "some_section"

; We should not internalize @deadfunc due to section
; CHECK: define void @deadfunc() section "some_other_section"
define void @deadfunc() section "some_other_section" {
  call void @deadfunc2()
  ret void
}

; We should not detect @deadfunc2 as dead (which would cause it to be
; internalized) since it is reached from a function with a section.
; CHECK2: define void @deadfunc2
declare void @deadfunc2()

; Dummy global to provide to -exported-symbol so that llvm-lto is happy and
; attempts internalization.
@foo = global i32 0
