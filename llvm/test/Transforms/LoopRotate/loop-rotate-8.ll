; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare i8* @foo()

define void @bar() #1 {
entry:
  br i1 undef, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  unreachable

if.end:                                           ; preds = %entry
  br i1 undef, label %return, label %for.cond

for.cond:                                         ; preds = %for.inc, %if.end
  br i1 undef, label %for.body, label %return

for.body:                                         ; preds = %for.cond
  %call15 = call i8* @foo()
  br i1 undef, label %if.end20, label %return

if.end20:                                         ; preds = %for.body
  %issuer.i = getelementptr inbounds i8, i8* %call15, i64 24
  br i1 undef, label %if.then.i, label %for.cond.i

if.then.i:                                        ; preds = %if.end20
  br i1 undef, label %if.then23, label %crl

for.cond.i:                                       ; preds = %if.end20
  br i1 undef, label %for.body.i, label %for.inc

for.body.i:                                       ; preds = %for.cond.i
  unreachable

crl:                    ; preds = %if.then.i
  br i1 undef, label %if.then23, label %for.inc

if.then23:                                        ; preds = %crl, %if.then.i
  %reason = getelementptr inbounds i8, i8* %call15, i64 32
  br label %return

for.inc:                                          ; preds = %crl, %for.cond.i
  br label %for.cond

return:                                           ; preds = %if.then23, %for.body, %for.cond, %if.end
  ret void
}
