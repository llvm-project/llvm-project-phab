; REQUIRES: x86
; RUN: llvm-as %s -o %t.o
; RUN: ld.lld %t.o -o %t.so -shared
; RUN: llvm-readelf -s %t.so | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@foo = hidden global i32 42, section "foo_section"
@bar = hidden global i32 42, section "bar_section"

@__start_foo_section = external global i32

define i32* @use() {
  ret i32* @__start_foo_section
}

; CHECK-NOT: bar_section
; CHECK:     foo_section PROGBITS
; CHECK-NOT: bar_section
