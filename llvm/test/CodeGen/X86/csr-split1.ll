; RUN: llc -mtriple=x86_64-unknown-linux-gnu < %s | FileCheck %s
; Check CSR split can work properly for tests below.

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

@cond = common local_unnamed_addr global i64 0, align 8
@ret = common local_unnamed_addr global i64 0, align 8
@p = common local_unnamed_addr global i64* null, align 8
declare void @foo(i64, i64, i64)

; After CSR split, ShrinkWrap is enabled and prologue are moved from entry
; to BB#1.
;
; CHECK-LABEL: test1:
; CHECK: .LBB0_1:
; CHECK-NEXT:   push
; CHECK-NEXT:   push
; CHECK-NEXT:   push
; CHECK:        callq foo
; CHECK:        pop
; CHECK-NEXT:   pop
; CHECK-NEXT:   pop
; CHECK-NEXT:   jmp .LBB0_2
define void @test1(i64 %i, i64 %j, i64 %k) nounwind {
entry:
  %t0 = load i64, i64* @cond, align 8
  %tobool = icmp eq i64 %t0, 0
  br i1 %tobool, label %if.end, label %if.then, !prof !0

if.then:                                          ; preds = %entry
  tail call void @foo(i64 %i, i64 %j, i64 %k)
  %add = add nsw i64 %j, %i
  %add1 = add nsw i64 %add, %k
  store i64 %add1, i64* @ret, align 8
  br label %if.end

if.end:                                           ; preds = %entry, %if.then
  %t1 = load i64*, i64** @p, align 8
  store volatile i64 3, i64* %t1, align 8
  ret void
}
!0 = !{!"branch_weights", i32 2000, i32 1}

; After CSR split, even if ShrinkWrap is not enabled because stack
; alloc space is used. param passing moves still can be moved from
; prologue to BB#1.
;
; CHECK-LABEL: test2:
; CHECK-NEXT: # BB#0:
; CHECK-NEXT:   pushq  %rbp
; CHECK-NEXT:   movq  %rsp, %rbp
; CHECK-NEXT:   pushq  %r15
; CHECK-NEXT:   pushq  %r14
; CHECK-NEXT:   pushq  %rbx
; CHECK-NEXT:   pushq  %rax
; CHECK-NEXT:   cmpq
; CHECK-NEXT:   jne .LBB1_1
; CHECK:      .LBB1_1:
; CHECK-NEXT:   movq  %rdi,
; CHECK-NEXT:   movq  %rsi,
; CHECK-NEXT:   movq  %rdx,
; CHECK-NEXT:   callq  foo
;
define void @test2(i64 %i, i64 %j, i64 %k) nounwind {
entry:
  %t0 = load i64, i64* @cond, align 8
  %tobool = icmp eq i64 %t0, 0
  br i1 %tobool, label %if.end, label %if.then, !prof !1

if.then:                                          ; preds = %entry
  tail call void @foo(i64 %i, i64 %j, i64 %k)
  %add = add nsw i64 %j, %i
  %add1 = add nsw i64 %add, %k
  store i64 %add1, i64* @ret, align 8
  br label %if.end

if.end:                                           ; preds = %entry, %if.then
  %t1 = alloca [3 x i8], align 16
  store [3 x i8]* %t1, [3 x i8]** bitcast (i64** @p to [3 x i8]**), align 8
  ret void
}
!1 = !{!"branch_weights", i32 2000, i32 1}
