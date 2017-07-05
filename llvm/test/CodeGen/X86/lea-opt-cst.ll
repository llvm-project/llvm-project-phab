; RUN: llc < %s -mtriple=x86_64-linux | FileCheck %s -check-prefix=X86
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.SA = type { i32 , i32  , i32 , i32 ,i32 }

define void @test_func(%struct.SA* nocapture %ctx, i32 %n) local_unnamed_addr {
; X86-LABEL: test_func: 
; X86:         # BB#0: 
; X86-NEXT:    movl (%rdi), %eax
; X86-NEXT:    movl 16(%rdi), %ecx
; X86-NEXT:    leal 1(%rax,%rcx), %eax
; X86-NEXT:    movl %eax, 12(%rdi)
; X86-NEXT:    leal (%eax,%rcx), %eax
; X86-NEXT:    movl %eax, 16(%rdi)
; X86-NEXT:    retq
 entry:
   %h0 = getelementptr inbounds %struct.SA, %struct.SA* %ctx, i64 0, i32 0
   %0 = load i32, i32* %h0, align 8
   %h3 = getelementptr inbounds %struct.SA, %struct.SA* %ctx, i64 0, i32 3
   %h4 = getelementptr inbounds %struct.SA, %struct.SA* %ctx, i64 0, i32 4
   %1 = load i32, i32* %h4, align 8
   %add = add i32 %0, 1
   %add4 = add i32 %add, %1
   store i32 %add4, i32* %h3, align 4
   %add29 = add i32 %add4 , %1
   store i32 %add29, i32* %h4, align 8
   ret void
}
