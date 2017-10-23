; RUN: llc < %s -mtriple=powerpc-unknown-linux-gnu -relocation-model=static | FileCheck --match-full-lines %s

declare i32 @call_foo(i32, ...)

define i32 @foo() {
entry:
  %call = call i32 (i32, ...) @call_foo(i32 0, i32 1)
  ret i32 0
}

; LABEL:foo:
; CHECK-NOT:  bl call_foo@PLT
; CHECK:      bl call_foo
