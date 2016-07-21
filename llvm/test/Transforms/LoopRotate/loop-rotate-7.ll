; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @bar() {
entry:
  br i1 undef, label %return, label %for.cond65

for.cond65:                                       ; preds = %entry, %for.body68
  %n.0.pn = phi i8* [ %n.1, %for.body68 ], [ undef, %entry ]
  %n.1 = getelementptr inbounds i8, i8* %n.0.pn, i64 -1
  br i1 undef, label %for.body68, label %return

for.body68:                                       ; preds = %for.cond65
  %xor71 = xor i8 undef, -1
  br label %for.cond65

return:                                           ; preds = %for.cond65, %entry
  ret i32 undef
}
