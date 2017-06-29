; RUN: llc -mtriple=aarch64-unknown-linux-gnu  < %s | FileCheck %s

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-linaro-linux-gnueabi"

; After splitting, instead of assigning CSR, ShrinkWrap is enabled and prologue
; are moved from entry to BB#1.

define i32 @test1(i32 %x, i32 %y, i32* nocapture %P) local_unnamed_addr {
; CHECK-LABEL: test1:
; CHECK-LABEL: BB#0:
; CHECK-NOT: stp
; CHECK-LABEL: BB#1:
; CHECK: stp x{{[0-9]+}}, x{{[0-9]+}}, [sp, {{.*}}]

entry:
  %idxprom = sext i32 %x to i64
  %arrayidx = getelementptr inbounds i32, i32* %P, i64 %idxprom
  %0 = load i32, i32* %arrayidx, align 4
  %idxprom1 = sext i32 %y to i64
  %arrayidx2 = getelementptr inbounds i32, i32* %P, i64 %idxprom1
  %1 = load i32, i32* %arrayidx2, align 4
  %add = add nsw i32 %1, %0
  %cmp = icmp eq i32 %add, 0
  br i1 %cmp, label %cleanup, label %if.end

if.end:                                           ; preds = %entry
  store i32 %add, i32* %P, align 4
  tail call void @func()
  br label %cleanup

cleanup:                                          ; preds = %entry, %if.end
  %retval.0 = phi i32 [ %x, %if.end ], [ 0, %entry ]
  ret i32 %retval.0
}

declare void @func()

