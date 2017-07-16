; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py

; RUN: llc < %s -mtriple=x86_64-unknown | FileCheck %s -check-prefix=X64
; RUN: llc < %s -mtriple=i686-unknown   | FileCheck %s -check-prefix=X86
%struct.SA = type { i32 , i32 , i32 , i32 , i32};

define void @foo(%struct.SA* nocapture %ctx, i32 %n) local_unnamed_addr #0 {
; X64-LABEL: foo:
; X64:       # BB#0: # %entry
; X64-NEXT:    .p2align 4, 0x90
; X64-NEXT:  .LBB0_1: # %loop
; X64-NEXT:    # =>This Inner Loop Header: Depth=1
; X64-NEXT:    movl (%rdi), %ecx
; X64-NEXT:    movl 16(%rdi), %eax
; X64-NEXT:    leal 1(%rcx,%rax), %ecx
; X64-NEXT:    movl %ecx, 12(%rdi)
; X64-NEXT:    decl %esi
; X64-NEXT:    jne .LBB0_1
; X64-NEXT:  # BB#2: # %exit
; X64-NEXT:    leal (%ecx,%rax), %eax
; X64-NEXT:    movl %eax, 16(%rdi)
; X64-NEXT:    retq
;
; X86-LABEL: foo:
; X86:       # BB#0: # %entry
; X86-NEXT:    pushl %esi
; X86-NEXT:  .Lcfi0:
; X86-NEXT:    .cfi_def_cfa_offset 8
; X86-NEXT:  .Lcfi1:
; X86-NEXT:    .cfi_offset %esi, -8
; X86-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    .p2align 4, 0x90
; X86-NEXT:  .LBB0_1: # %loop
; X86-NEXT:    # =>This Inner Loop Header: Depth=1
; X86-NEXT:    movl (%eax), %esi
; X86-NEXT:    movl 16(%eax), %edx
; X86-NEXT:    leal 1(%esi,%edx), %esi
; X86-NEXT:    movl %esi, 12(%eax)
; X86-NEXT:    decl %ecx
; X86-NEXT:    jne .LBB0_1
; X86-NEXT:  # BB#2: # %exit
; X86-NEXT:    leal (%esi,%edx), %ecx
; X86-NEXT:    movl %ecx, 16(%eax)
; X86-NEXT:    popl %esi
; X86-NEXT:    retl
 entry:
   br label %loop

 loop:
   %iter = phi i32 [%n ,%entry ] ,[ %iter.ctr ,%loop]
   %h0 = getelementptr inbounds %struct.SA, %struct.SA* %ctx, i64 0, i32 0
   %0 = load i32, i32* %h0, align 8
   %h3 = getelementptr inbounds %struct.SA, %struct.SA* %ctx, i64 0, i32 3
   %h4 = getelementptr inbounds %struct.SA, %struct.SA* %ctx, i64 0, i32 4
   %1 = load i32, i32* %h4, align 8
   %add = add i32 %0, 1
   %add4 = add i32 %add, %1
   store i32 %add4, i32* %h3, align 4
   %add29 = add i32 %add4, %1
   %iter.ctr = sub i32 %iter , 1
   %res = icmp ne i32 %iter.ctr , 0
   br i1 %res , label %loop , label %exit

 exit:
   store i32 %add29, i32* %h4, align 8
   ret void
}
