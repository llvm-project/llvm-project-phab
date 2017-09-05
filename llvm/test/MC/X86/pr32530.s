// RUN: llvm-mc -triple x86_64-unknown-unknown -x86-asm-syntax=intel %s | FileCheck %s

// CHECK: movq $msg, %rsi
.text
  mov rsi, offset msg
.data
msg:
  .ascii "Hello, world!\n"
