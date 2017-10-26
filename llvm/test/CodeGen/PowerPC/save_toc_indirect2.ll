; RUN: llc -verify-machineinstrs -mtriple=powerpc64le-unknown-linux-gnu -mattr=+save-toc-indirect < %s | FileCheck %s --check-prefix=TOC_INDIRECT

define signext i32 @test(i32 signext %i, i32 (i32)* nocapture %Func, i32 (i32)* nocapture %Func2) {
; TOC_INDIRECT-LABEL: test:
; TOC_INDIRECT:       # BB#0: # %entry
; TOC_INDIRECT:    std 2,  {{[0-9]+}}(1)
; TOC_INDIRECT:    bctrl
; TOC_INDIRECT:    ld 2, {{[0-9]+}}(1)
; TOC_INDIRECT:    std 2, {{[0-9]+}}(1)
; TOC_INDIRECT:    bctrl
; TOC_INDIRECT:    ld 2, {{[0-9]+}}(1)
; TOC_INDIRECT:    std 2, {{[0-9]+}}(1)
; TOC_INDIRECT:    bctrl
; TOC_INDIRECT:    ld 2, {{[0-9]+}}(1)
entry:
  %call = tail call signext i32 %Func(i32 signext %i)
  %call1 = tail call signext i32 %Func2(i32 signext %i)
  %add2 = add nsw i32 %call1, %call
  %conv = sext i32 %add2 to i64
  %0 = alloca i8, i64 %conv, align 16
  %1 = bitcast i8* %0 to i32*
  %cmp24 = icmp sgt i32 %add2, 0
  br i1 %cmp24, label %for.body.lr.ph, label %for.cond.cleanup

for.body.lr.ph:                                   ; preds = %entry
  %wide.trip.count = zext i32 %add2 to i64
  br label %for.body

for.cond.cleanup:                                 ; preds = %for.body, %entry
  %call7 = call signext i32 @UseAlloca(i32* nonnull %1)
  %add8 = add nsw i32 %call7, %add2
  ret i32 %add8

for.body:                                         ; preds = %for.body, %for.body.lr.ph
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %2 = add nsw i64 %indvars.iv, %conv
  %3 = trunc i64 %2 to i32
  %call6 = tail call signext i32 %Func(i32 signext %3)
  %arrayidx = getelementptr inbounds i32, i32* %1, i64 %indvars.iv
  store i32 %call6, i32* %arrayidx, align 4
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.cond.cleanup, label %for.body
}

declare signext i32 @UseAlloca(i32*)
