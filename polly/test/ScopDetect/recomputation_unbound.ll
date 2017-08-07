; RUN: opt -polly-import-jscop -polly-import-jscop-dir=%S -polly-import-jscop-postfix=unbound -polly-ast -debug -polly-process-unprofitable -disable-polly-legality < %s 2>&1 | FileCheck %s

;void double_convolution(int* restrict Input, int* restrict Kernel1, int* restrict Intermediate,
;			int* restrict Kernel2, int* restrict Output){
;
;	for (int m1 = 0; m1 < 50; m1++){
;		Intermediate[m1] = 0;
;	}
;
;	for (int m1 = 0; m1 < 48; m1++){
;		Output[m1] = 0;
;	}
;	
;	for (int m1 = 0; m1 < 50; m1++){
;		for(int k1 = 0; k1 < 3; k1++){
;			Intermediate[m1] += Input[ m1 + k1 ] * Kernel1[k1];
;		}
;	}
;
;	for (int m2 = 0; m2 < 48; m2++){
;		for(int k2 = 0; k2 < 3; k2++){
;			Output[m2] += Intermediate[ m2 + k2 ] * Kernel2[k2];
;		}
;	}
;}

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @double_convolution(i32* noalias %Input, i32* noalias %Kernel1, i32* noalias %Intermediate, i32* noalias %Kernel2, i32* noalias %Output) #0 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %entry.split, %for.body
  %indvars.iv21 = phi i64 [ 0, %entry.split ], [ %indvars.iv.next22, %for.body ]
  %arrayidx = getelementptr inbounds i32, i32* %Intermediate, i64 %indvars.iv21
  store i32 0, i32* %arrayidx, align 4
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1
  %exitcond23 = icmp ne i64 %indvars.iv.next22, 50
  br i1 %exitcond23, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  br label %for.body4

for.body4:                                        ; preds = %for.end, %for.body4
  %indvars.iv18 = phi i64 [ 0, %for.end ], [ %indvars.iv.next19, %for.body4 ]
  %arrayidx6 = getelementptr inbounds i32, i32* %Output, i64 %indvars.iv18
  store i32 0, i32* %arrayidx6, align 4
  %indvars.iv.next19 = add nuw nsw i64 %indvars.iv18, 1
  %exitcond20 = icmp ne i64 %indvars.iv.next19, 48
  br i1 %exitcond20, label %for.body4, label %for.end9

for.end9:                                         ; preds = %for.body4
  br label %for.body13

for.body13:                                       ; preds = %for.end9, %for.inc27
  %indvars.iv15 = phi i64 [ 0, %for.end9 ], [ %indvars.iv.next16, %for.inc27 ]
  br label %for.body16

for.body16:                                       ; preds = %for.body13, %for.body16
  %indvars.iv11 = phi i64 [ 0, %for.body13 ], [ %indvars.iv.next12, %for.body16 ]
  %0 = add nuw nsw i64 %indvars.iv11, %indvars.iv15
  %arrayidx18 = getelementptr inbounds i32, i32* %Input, i64 %0
  %1 = load i32, i32* %arrayidx18, align 4
  %arrayidx20 = getelementptr inbounds i32, i32* %Kernel1, i64 %indvars.iv11
  %2 = load i32, i32* %arrayidx20, align 4
  %mul = mul nsw i32 %2, %1
  %arrayidx22 = getelementptr inbounds i32, i32* %Intermediate, i64 %indvars.iv15
  %3 = load i32, i32* %arrayidx22, align 4
  %add23 = add nsw i32 %3, %mul
  store i32 %add23, i32* %arrayidx22, align 4
  %indvars.iv.next12 = add nuw nsw i64 %indvars.iv11, 1
  %exitcond14 = icmp ne i64 %indvars.iv.next12, 3
  br i1 %exitcond14, label %for.body16, label %for.inc27

for.inc27:                                        ; preds = %for.body16
  %indvars.iv.next16 = add nuw nsw i64 %indvars.iv15, 1
  %exitcond17 = icmp ne i64 %indvars.iv.next16, 50
  br i1 %exitcond17, label %for.body13, label %for.end29

for.end29:                                        ; preds = %for.inc27
  br label %for.body32

for.body32:                                       ; preds = %for.end29, %for.inc48
  %indvars.iv8 = phi i64 [ 0, %for.end29 ], [ %indvars.iv.next9, %for.inc48 ]
  br label %for.body35

for.body35:                                       ; preds = %for.body32, %for.body35
  %indvars.iv = phi i64 [ 0, %for.body32 ], [ %indvars.iv.next, %for.body35 ]
  %4 = add nuw nsw i64 %indvars.iv, %indvars.iv8
  %arrayidx38 = getelementptr inbounds i32, i32* %Intermediate, i64 %4
  %5 = load i32, i32* %arrayidx38, align 4
  %arrayidx40 = getelementptr inbounds i32, i32* %Kernel2, i64 %indvars.iv
  %6 = load i32, i32* %arrayidx40, align 4
  %mul41 = mul nsw i32 %6, %5
  %arrayidx43 = getelementptr inbounds i32, i32* %Output, i64 %indvars.iv8
  %7 = load i32, i32* %arrayidx43, align 4
  %add44 = add nsw i32 %7, %mul41
  store i32 %add44, i32* %arrayidx43, align 4
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp ne i64 %indvars.iv.next, 3
  br i1 %exitcond, label %for.body35, label %for.inc48

for.inc48:                                        ; preds = %for.body35
  %indvars.iv.next9 = add nuw nsw i64 %indvars.iv8, 1
  %exitcond10 = icmp ne i64 %indvars.iv.next9, 48
  br i1 %exitcond10, label %for.body32, label %for.end50

for.end50:                                        ; preds = %for.inc48
  ret void
}

; CHECK: JScop file contains a schedule that is not single valued
; CHECK: JScop file contains a schedule that is not bounded
