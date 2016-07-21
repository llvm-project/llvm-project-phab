; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info
; ModuleID = 'bugpoint-reduced-instructions.bc'
source_filename = "bugpoint-output-cebc3aa.bc"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @bar() {
entry:
  br i1 undef, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 -1

if.end:                                           ; preds = %entry
  br i1 undef, label %if.then39, label %if.end48

if.then39:                                        ; preds = %if.end
  unreachable

if.end48:                                         ; preds = %if.end
  br i1 undef, label %if.else61, label %if.then59

if.then59:                                        ; preds = %if.end48
  unreachable

if.else61:                                        ; preds = %if.end48
  br i1 undef, label %if.then91, label %if.end92

if.then91:                                        ; preds = %if.else61
  unreachable

if.end92:                                         ; preds = %if.else61
  br i1 undef, label %if.then96, label %while.cond

if.then96:                                        ; preds = %if.end92
  unreachable

while.cond:                                       ; preds = %if.end107, %if.end92
  %len.3 = phi i64 [ %add109, %if.end107 ], [ 0, %if.end92 ]
  br i1 undef, label %if.then106, label %if.end107

if.then106:                                       ; preds = %while.cond
  unreachable

if.end107:                                        ; preds = %while.cond
  %add109 = add i64 undef, %len.3
  %sub111 = sub i64 undef, undef
  br label %while.cond
}
