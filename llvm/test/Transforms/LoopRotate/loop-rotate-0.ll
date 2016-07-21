; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @a2i_ASN1_ENUMERATED() {
entry:
  br label %for.cond

for.cond:                                         ; preds = %if.end94, %entry
  %first.0 = phi i32 [ 1, %entry ], [ 0, %if.end94 ]
  br i1 undef, label %err_sl, label %if.end

if.end:                                           ; preds = %for.cond
  br i1 undef, label %err_sl, label %if.end75

if.end75:                                         ; preds = %if.end
  br i1 undef, label %if.end94, label %if.then93

if.then93:                                        ; preds = %if.end75
  ret i32 0

if.end94:                                         ; preds = %if.end75
  %call179 = tail call i32 @gets()
  br label %for.cond

err_sl:                                           ; preds = %if.end, %for.cond
  unreachable
}

declare i32 @gets()
