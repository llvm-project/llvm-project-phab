; RUN: opt %loadPolly -polly-canonicalize -polly-mse -analyze < %s 2>1| FileCheck %s
;
; Verify that the accesses are correctly expanded for MemoryKind::PHI
;
; Original source code :
;
; #define Ni 10000
; #define Nj 10000
;
; void mse(double A[Ni], double B[Nj]) {
;   int i,j;
;   double tmp = 6;
;   for (i = 0; i < Ni; i++) {
;     for (int j = 0; j<Nj; j++) {
;       tmp = tmp + 2;
;     }
;     B[i] = tmp;
;   }
;
; CHECK: There are more than one RAW dependencies in the union map.
;

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind  uwtable
define void @mse(double* %A, double* %B) {
entry:
  %A.addr = alloca double*, align 8
  %B.addr = alloca double*, align 8
  %i = alloca i32, align 4
  %j = alloca i32, align 4
  %tmp = alloca double, align 8
  %j1 = alloca i32, align 4
  store double* %A, double** %A.addr, align 8
  store double* %B, double** %B.addr, align 8
  store double 6.000000e+00, double* %tmp, align 8
  store i32 0, i32* %i, align 4
  br label %for.cond
for.cond:                                         ; preds = %for.inc8, %entry
  %0 = load i32, i32* %i, align 4
  %cmp = icmp slt i32 %0, 10000
  br i1 %cmp, label %for.body, label %for.end10
for.body:                                         ; preds = %for.cond
  %1 = load i32, i32* %i, align 4
  %conv = sitofp i32 %1 to double
  store double %conv, double* %tmp, align 8
  store i32 0, i32* %j1, align 4
  br label %for.cond2
for.cond2:                                        ; preds = %for.inc, %for.body
  %2 = load i32, i32* %j1, align 4
  %cmp3 = icmp slt i32 %2, 10000
  br i1 %cmp3, label %for.body5, label %for.end
for.body5:                                        ; preds = %for.cond2
  %3 = load double, double* %tmp, align 8
  %add = fadd double %3, 3.000000e+00
  %4 = load double*, double** %A.addr, align 8
  %5 = load i32, i32* %j1, align 4
  %idxprom = sext i32 %5 to i64
  %arrayidx = getelementptr inbounds double, double* %4, i64 %idxprom
  store double %add, double* %arrayidx, align 8
  br label %for.inc
for.inc:                                          ; preds = %for.body5
  %6 = load i32, i32* %j1, align 4
  %inc = add nsw i32 %6, 1
  store i32 %inc, i32* %j1, align 4
  br label %for.cond2
for.end:                                          ; preds = %for.cond2
  %7 = load double, double* %tmp, align 8
  %8 = load double*, double** %B.addr, align 8
  %9 = load i32, i32* %i, align 4
  %idxprom6 = sext i32 %9 to i64
  %arrayidx7 = getelementptr inbounds double, double* %8, i64 %idxprom6
  store double %7, double* %arrayidx7, align 8
  br label %for.inc8
for.inc8:                                         ; preds = %for.end
  %10 = load i32, i32* %i, align 4
  %inc9 = add nsw i32 %10, 1
  store i32 %inc9, i32* %i, align 4
  br label %for.cond
for.end10:                                        ; preds = %for.cond
  ret void
}
