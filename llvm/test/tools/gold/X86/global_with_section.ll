; Test to ensure we don't internalize or treat as dead a global value
; with a section. Otherwise, ELF linker generation of __start_"sectionname"
; and __stop_"sectionname" symbols would not occur and we can end up
; with undefined references at link time.

; RUN: opt %s -o %t.o
; RUN: llvm-lto2 dump-symtab %t.o | FileCheck %s --check-prefix=SYMTAB
; RUN: opt %p/Inputs/global_with_section.ll -o %t2.o
; RUN: %gold -m elf_x86_64 -plugin %llvmshlibdir/LLVMgold.so \
; RUN:     -u foo \
; RUN:     --plugin-opt=save-temps \
; RUN:     -o %t3.o %t.o %t2.o
; Check results of internalization
; RUN: llvm-dis %t3.o.0.2.internalize.bc -o - | FileCheck %s --check-prefix=CHECK2-REGULARLTO

; Do setup work for all below tests: generate bitcode
; RUN: opt -module-summary %s -o %t.o
; RUN: llvm-lto2 dump-symtab %t.o | FileCheck %s --check-prefix=SYMTAB
; RUN: opt -module-summary %p/Inputs/global_with_section.ll -o %t2.o

; RUN: %gold -m elf_x86_64 -plugin %llvmshlibdir/LLVMgold.so \
; RUN:     -u foo \
; RUN:     --plugin-opt=thinlto \
; RUN:     --plugin-opt=save-temps \
; RUN:     -o %t3.o %t.o %t2.o
; Check results of internalization
; RUN: llvm-dis %t.o.2.internalize.bc -o - | FileCheck %s
; RUN: llvm-dis %t2.o.2.internalize.bc -o - | FileCheck %s --check-prefix=CHECK2-THINLTO

; SYMTAB: deadfunc
; SYMTAB-NEXT: has C idenfier section name
; SYMTAB-NEXT: deadfunc2
; SYMTAB-NEXT: var_with_section
; SYMTAB-NEXT: has C idenfier section name

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; We should not internalize @var_with_section due to section
; CHECK-DAG: @var_with_section = global i32 0, section "some_section"
@var_with_section = global i32 0, section "some_section"

; We should not internalize @deadfunc due to section
; CHECK-DAG: define void @deadfunc() section "some_other_section"
define void @deadfunc() section "some_other_section" {
  call void @deadfunc2()
  ret void
}

; In RegularLTO mode, where we have combined all the IR, @deadfunc2
; should still exist but would be internalized.
; CHECK2-REGULARLTO: define internal void @deadfunc2
; In ThinLTO mode, we can't internalize it as it needs to be preserved
; (due to the access from @deadfunc which must be preserved), and
; can't be internalized since the reference is from a different module.
; CHECK2-THINLTO: define void @deadfunc2
declare void @deadfunc2()

@foo = global i32 0
