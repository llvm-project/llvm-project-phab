; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare i32 @foo()
define i32 @bar() {
entry:
  br label %for.cond

for.cond:                                         ; preds = %if.then178, %entry
  %first.0 = phi i32 [ 1, %entry ], [ 0, %if.then178 ]
  br i1 undef, label %err_sl, label %if.end

if.end:                                           ; preds = %for.cond
  br i1 undef, label %err_sl, label %if.end75

if.end75:                                         ; preds = %if.end
  br i1 undef, label %if.end94, label %if.then93

if.then93:                                        ; preds = %if.end75
  unreachable

if.end94:                                         ; preds = %if.end75
  br i1 undef, label %if.then178, label %for.end182

if.then178:                                       ; preds = %if.end94
  %call179 = tail call i32 @foo()
  br label %for.cond

for.end182:                                       ; preds = %if.end94
  ret i32 1

err_sl:                                           ; preds = %if.end, %for.cond
  unreachable
}
