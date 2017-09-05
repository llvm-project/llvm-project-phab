; RUN: llc -mtriple=x86_64-unknown-linux-gnu -x86-asm-syntax=intel < %s | FileCheck %s

@.str = private unnamed_addr constant [1 x i8] zeroinitializer, align 1

; Test we are emitting the 'offset' operator upon an immediate reference of a label:
; The emitted 'att-equivalent' of this one is "movl $.L.str, %edi"

; CHECK: mov     edi, offset .L.str
define void @g(i8* %s) {
entry:
  %s.addr = alloca i8*, align 8
  %a = alloca i8*, align 8
  store i8* %s, i8** %s.addr, align 8
  store i8* getelementptr inbounds ([1 x i8], [1 x i8]* @.str, i32 0, i32 0), i8** %a, align 8
  %0 = load i8*, i8** %a, align 8
  call void @f(i8* %0)
  ret void
}

declare void @f(i8*) #1
