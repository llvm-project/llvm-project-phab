; RUN: opt -basicaa -polly-import-jscop -polly-import-jscop-dir=%S -polly-import-jscop-postfix=notbound -polly-ast -analyze -disable-polly-legality -debug < %s 2>&1 | FileCheck %s

; void double_convolution(unsigned char* restrict l1_ucdata, int* restrict k1_idata, unsigned char* restrict l2_ucdata,
;                         int* restrict k2_idata, unsigned char* restrict l3_ucdata) {
;
;  for (int m1 = 0; m1 < 96; m1++){
;    int output1=0;
;    for(int k1 = 0; k1 < 5; k1++){
;      output1 += ((int) l1_ucdata[ m1 + k1 ]) * k1_idata[k1];
;    }
;    l2_ucdata[m1]= output1;
;  }
;
;  for (int m2 = 0; m2 < 94; m2++){
;    int output2=0;
;    for(int k2 = 0; k2 < 3; k2++){
;      output2 += ((int) l2_ucdata[ m2 + k2 ]) * k2_idata[k2];
;    }
;    l3_ucdata[m2]= output2;
;  }
;}

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @double_convolution(i8* noalias %l1_ucdata, i32* noalias %k1_idata, i8* noalias %l2_ucdata, i32* noalias %k2_idata, i8* noalias %l3_ucdata) #0 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  br label %for.cond1.preheader

for.cond1.preheader:                              ; preds = %entry.split, %for.end
  %indvars.iv15 = phi i64 [ 0, %entry.split ], [ %indvars.iv.next16, %for.end ]
  br label %for.inc

for.cond13.preheader:                             ; preds = %for.end
  br label %for.cond17.preheader

for.inc:                                          ; preds = %for.cond1.preheader, %for.inc
  %indvars.iv11 = phi i64 [ 0, %for.cond1.preheader ], [ %indvars.iv.next12, %for.inc ]
  %output1.04 = phi i32 [ 0, %for.cond1.preheader ], [ %add6, %for.inc ]
  %arrayidx5 = getelementptr inbounds i32, i32* %k1_idata, i64 %indvars.iv11
  %0 = load i32, i32* %arrayidx5, align 4
  %1 = add nuw nsw i64 %indvars.iv11, %indvars.iv15
  %arrayidx = getelementptr inbounds i8, i8* %l1_ucdata, i64 %1
  %2 = load i8, i8* %arrayidx, align 1
  %conv = zext i8 %2 to i32
  %mul = mul nsw i32 %0, %conv
  %add6 = add nsw i32 %mul, %output1.04
  %indvars.iv.next12 = add nuw nsw i64 %indvars.iv11, 1
  %exitcond14 = icmp ne i64 %indvars.iv.next12, 5
  br i1 %exitcond14, label %for.inc, label %for.end

for.end:                                          ; preds = %for.inc
  %add6.lcssa = phi i32 [ %add6, %for.inc ]
  %conv7 = trunc i32 %add6.lcssa to i8
  %arrayidx9 = getelementptr inbounds i8, i8* %l2_ucdata, i64 %indvars.iv15
  store i8 %conv7, i8* %arrayidx9, align 1
  %indvars.iv.next16 = add nuw nsw i64 %indvars.iv15, 1
  %exitcond17 = icmp ne i64 %indvars.iv.next16, 96
  br i1 %exitcond17, label %for.cond1.preheader, label %for.cond13.preheader

for.cond17.preheader:                             ; preds = %for.cond13.preheader, %for.end31
  %indvars.iv8 = phi i64 [ 0, %for.cond13.preheader ], [ %indvars.iv.next9, %for.end31 ]
  br label %for.inc29

for.inc29:                                        ; preds = %for.cond17.preheader, %for.inc29
  %indvars.iv = phi i64 [ 0, %for.cond17.preheader ], [ %indvars.iv.next, %for.inc29 ]
  %output2.01 = phi i32 [ 0, %for.cond17.preheader ], [ %add28, %for.inc29 ]
  %arrayidx26 = getelementptr inbounds i32, i32* %k2_idata, i64 %indvars.iv
  %3 = load i32, i32* %arrayidx26, align 4
  %4 = add nuw nsw i64 %indvars.iv, %indvars.iv8
  %arrayidx23 = getelementptr inbounds i8, i8* %l2_ucdata, i64 %4
  %5 = load i8, i8* %arrayidx23, align 1
  %conv24 = zext i8 %5 to i32
  %mul27 = mul nsw i32 %3, %conv24
  %add28 = add nsw i32 %mul27, %output2.01
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp ne i64 %indvars.iv.next, 3
  br i1 %exitcond, label %for.inc29, label %for.end31

for.end31:                                        ; preds = %for.inc29
  %add28.lcssa = phi i32 [ %add28, %for.inc29 ]
  %conv32 = trunc i32 %add28.lcssa to i8
  %arrayidx34 = getelementptr inbounds i8, i8* %l3_ucdata, i64 %indvars.iv8
  store i8 %conv32, i8* %arrayidx34, align 1
  %indvars.iv.next9 = add nuw nsw i64 %indvars.iv8, 1
  %exitcond10 = icmp ne i64 %indvars.iv.next9, 94
  br i1 %exitcond10, label %for.cond17.preheader, label %for.end37

for.end37:                                        ; preds = %for.end31
  ret void
}


; CHECK: JScop file contains a schedule that is not single valued
; CHECK: JScop file contains a schedule that is not bounded
; CHECK-NOT: Stmt_for_inc29_copy_no_1
; CHECK-NOT: Stmt_for_inc_copy_no_1
