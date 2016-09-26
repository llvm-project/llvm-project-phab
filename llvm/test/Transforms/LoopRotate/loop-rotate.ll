; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck %s
; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck -check-prefix=CHECK1 %s
; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck -check-prefix=CHECK2 %s
; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck -check-prefix=CHECK3 %s
; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck -check-prefix=CHECK4 %s
; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck -check-prefix=CHECK5 %s
; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck -check-prefix=CHECK6 %s
; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck -check-prefix=CHECK7 %s
; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck -check-prefix=CHECK8 %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Check that the loop is rotated.
; CHECK-LABEL: bar0
; CHECK: while.cond.lr:                                    ; preds = %entry
; CHECK: while.body.lr.ph:                                 ; preds = %while.cond.lr

%fp = type { i64 }

declare void @foo0(%fp* %this, %fp* %that)

define void @bar0(%fp* %__begin1, %fp* %__end1, %fp** %__end2) {
entry:
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %__end1.addr.0 = phi %fp* [ %__end1, %entry ], [ %incdec.ptr, %while.body ]
  %cmp = icmp eq %fp* %__end1.addr.0, %__begin1
  br i1 %cmp, label %while.end, label %while.body

while.body:                                       ; preds = %while.cond
  %0 = load %fp*, %fp** %__end2, align 8
  %add.ptr = getelementptr inbounds %fp, %fp* %0, i64 -1
  %incdec.ptr = getelementptr inbounds %fp, %fp* %__end1.addr.0, i64 -1
  tail call void @foo0(%fp* %add.ptr, %fp* %incdec.ptr)
  %1 = load %fp*, %fp** %__end2, align 8
  %incdec.ptr2 = getelementptr inbounds %fp, %fp* %1, i64 -1
  store %fp* %incdec.ptr2, %fp** %__end2, align 8
  br label %while.cond

while.end:                                        ; preds = %while.cond
  ret void
}


; Check that the loop is rotated.
; CHECK1-LABEL: bar1
; CHECK1: for.cond.lr:                                      ; preds = %entry
; CHECK1: if.end.lr:                                        ; preds = %for.cond.lr
; CHECK1: if.end75.lr:                                      ; preds = %if.end.lr
; CHECK1: if.end94.lr:                                      ; preds = %if.end75.lr
; CHECK1: if.then178.lr.ph:                                 ; preds = %if.end94.lr

declare i32 @foo1()
define i32 @bar1() {
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
  %call179 = tail call i32 @foo1()
  br label %for.cond

for.end182:                                       ; preds = %if.end94
  ret i32 1

err_sl:                                           ; preds = %if.end, %for.cond
  unreachable
}


; Check that the loop is rotated and multiple phis are updated.
; CHECK2-LABEL: foo2
; CHECK2: do.body.i10.lr:                                   ; preds = %if.else.i3
; CHECK2: do.cond.i19.lr.ph:                                ; preds = %do.body.i10.lr

define void @foo2() {
entry:
  br i1 undef, label %for.body, label %for.end

for.body:                                         ; preds = %entry
  br i1 undef, label %inverse.exit21, label %if.else.i3

if.else.i3:                                       ; preds = %for.body
  br label %do.body.i10

do.body.i10:                                      ; preds = %do.cond.i19, %if.else.i3
  %b1.0.i4 = phi i64 [ 0, %if.else.i3 ], [ %b2.0.i7, %do.cond.i19 ]
  %b2.0.i7 = phi i64 [ 1, %if.else.i3 ], [ %sub8.i18, %do.cond.i19 ]
  br i1 undef, label %do.cond.thread.i15, label %do.cond.i19

do.cond.thread.i15:                               ; preds = %do.body.i10
  br label %inverse.exit21

do.cond.i19:                                      ; preds = %do.body.i10
  %mul.i17 = mul nsw i64 undef, %b2.0.i7
  %sub8.i18 = sub nsw i64 %b1.0.i4, %mul.i17
  br label %do.body.i10

inverse.exit21:                                   ; preds = %do.cond.thread.i15, %for.body
  br label %for.end

for.end:                                          ; preds = %inverse.exit21, %entry
  ret void
}

; Check that the loop with indirect branches is rotated.
; CHECK3-LABEL: foo3
; CHECK3: while.cond.lr:                                    ; preds = %entry
; CHECK3: if.end6.lr:                                       ; preds = %while.cond.lr
; CHECK3: if.else.lr:                                       ; preds = %if.end6.lr
; CHECK3: if.end2.i.lr:                                     ; preds = %if.else.lr
; CHECK3: if.end37.lr:                                      ; preds = %if.end2.i.lr
; CHECK3: if.else96.lr:                                     ; preds = %if.end37.lr
; CHECK3: if.else106.lr.ph:                                 ; preds = %if.else96.lr

define i32 @foo3() {
entry:
  br label %while.cond

while.cond:                                       ; preds = %if.then467, %entry
  br i1 undef, label %if.then, label %if.end6

if.then:                                          ; preds = %while.cond
  unreachable

if.end6:                                          ; preds = %while.cond
  %conv7 = ashr exact i64 undef, 32
  br i1 undef, label %if.else, label %if.then19

if.then19:                                        ; preds = %if.end6
  unreachable

if.else:                                          ; preds = %if.end6
  br i1 undef, label %thr, label %if.end2.i

if.end2.i:                                        ; preds = %if.else
  br i1 undef, label %thr, label %if.end37

thr:                                              ; preds = %if.end2.i, %if.else
  unreachable

if.end37:                                         ; preds = %if.end2.i
  br i1 undef, label %if.else96, label %if.then40

if.then40:                                        ; preds = %if.end37
  unreachable

if.else96:                                        ; preds = %if.end37
  br i1 undef, label %if.else106, label %if.then99

if.then99:                                        ; preds = %if.else96
  unreachable

if.else106:                                       ; preds = %if.else96
  switch i32 undef, label %if.else427 [
    i32 12, label %if.then130
    i32 30, label %if.then467
  ]

if.then130:                                       ; preds = %if.else106
  br label %if.then467

if.else427:                                       ; preds = %if.else106
  unreachable

if.then467:                                       ; preds = %if.then130, %if.else106
  br label %while.cond
}

; Check that the loop is rotated.
; CHECK4-LABEL: foo4
; CHECK4: bb3.lr:                                           ; preds = %bb
; CHECK4: bb5.lr:                                           ; preds = %bb3.lr
; CHECK4: bb8.lr.ph:                                        ; preds = %bb5.lr

define void @foo4(i8* %arg, i8* %arg1, i64 %arg2) {
bb:
  %tmp = and i32 undef, 7
  br label %bb3

bb3:                                              ; preds = %bb8, %bb
  %tmp4 = phi i32 [ 0, %bb8 ], [ %tmp, %bb ]
  br i1 undef, label %bb9, label %bb5

bb5:                                              ; preds = %bb3
  %tmp6 = sub nsw i32 8, %tmp4
  br i1 undef, label %bb7, label %bb8

bb7:                                              ; preds = %bb5
  unreachable

bb8:                                              ; preds = %bb5
  store i32 undef, i32* undef, align 8
  br label %bb3

bb9:                                              ; preds = %bb3
  ret void
}


; Check that the loop with pre-header and loop-body having a switch block is rotated.
; CHECK5-LABEL: bar5
; CHECK5: bb19.preheader:                                   ; preds = %bb10, %bb10
; CHECK5: bb19.lr:                                          ; preds = %bb19.preheader
; CHECK5: bb12.lr.ph:                                       ; preds = %bb19.lr

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare i32 @foo5(i8*, i32)

define i32 @bar5(i8* %arg, i32 %arg1, i8* %arg2, i32 %arg3) {
bb6:
  br i1 undef, label %bb8, label %bb10

bb8:                                              ; preds = %bb6
  unreachable

bb10:                                             ; preds = %bb6
  switch i32 undef, label %bb11 [
    i32 46, label %bb19
    i32 32, label %bb19
  ]

bb11:                                             ; preds = %bb10
  unreachable

bb12:                                             ; preds = %bb19
  switch i8 undef, label %bb13 [
    i8 32, label %bb21
    i8 46, label %bb21
  ]

bb13:                                             ; preds = %bb12
  br label %bb15

bb15:                                             ; preds = %bb13
  br i1 undef, label %bb17, label %bb16

bb16:                                             ; preds = %bb15
  %tmp = call i32 @foo5(i8* %tmp20, i32 10)
  ret i32 0

bb17:                                             ; preds = %bb15
  %tmp18 = phi i8* [ %tmp20, %bb15 ]
  br label %bb19

bb19:                                             ; preds = %bb17, %bb10, %bb10
  %tmp20 = phi i8* [ %tmp18, %bb17 ], [ null, %bb10 ], [ null, %bb10 ]
  br i1 undef, label %bb21, label %bb12

bb21:                                             ; preds = %bb19, %bb12, %bb12
  unreachable
}


; Check that the loop is rotated.
; CHECK6-LABEL: bar6
; CHECK6: while.cond.preheader:                             ; preds = %if.end92
; CHECK6: while.cond.lr:                                    ; preds = %while.cond.preheader
; CHECK6: if.end107.lr.ph:                                  ; preds = %while.cond.lr

; Function Attrs: nounwind uwtable
define i32 @bar6() {
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


declare i8* @foo7()

; Check that the loop is rotated.
; CHECK7-LABEL: bar7
; CHECK7: for.cond.preheader:                               ; preds = %if.end
; CHECK7: for.cond.lr:                                      ; preds = %for.cond.preheader
; CHECK7: for.body.lr:                                      ; preds = %for.cond.lr
; CHECK7: if.end20.lr.ph:                                   ; preds = %for.body.lr

define void @bar7() #1 {
entry:
  br i1 undef, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  unreachable

if.end:                                           ; preds = %entry
  br i1 undef, label %return, label %for.cond

for.cond:                                         ; preds = %for.inc, %if.end
  br i1 undef, label %for.body, label %return

for.body:                                         ; preds = %for.cond
  %call15 = call i8* @foo7()
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


; Check that the loop with an if-block within the loop is rotated.
; There are multiple exits from the loop.

; CHECK8-LABEL: foo8
; CHECK8: for.cond.lr:                                      ; preds = %entry
; CHECK8: %first.0.lr = phi i32 [ 1, %entry ]
; CHECK8: if.end.lr:                                        ; preds = %for.cond.lr
; CHECK8: if.end75.lr:                                      ; preds = %if.end.lr
; CHECK8: if.end94.lr.ph:                                   ; preds = %if.end75.lr

define i32 @foo8() {
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
