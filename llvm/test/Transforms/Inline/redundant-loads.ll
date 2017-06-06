; RUN: opt -inline < %s -S -o - -inline-threshold=225 | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.matrix = type { [3 x [3 x double]] }

define void @outer(%struct.matrix* %a, %struct.matrix* %b) {
; CHECK-LABEL: @outer(
; CHECK-NOT: call void @matrix_multiply
  %c = alloca %struct.matrix
  call void @matrix_multiply(%struct.matrix* %a, %struct.matrix* %b, %struct.matrix* %c)
  ret void
}

; Every data in matrix a and b are loaded three times.
; typedef struct { double m[3][3]; } matrix;
; void matrix_multiply( matrix *a, matrix *b, matrix *c ){
;   int i,j,k;
;     for(i=0; i<3; i++)
;       for(j=0; j<3; j++)
;          for(k=0; k<3; k++)
;            c->m[i][j] += a->m[i][k] * b->m[k][j];
; }

define void @matrix_multiply(%struct.matrix* %a, %struct.matrix* %b, %struct.matrix* %c) {
entry:
  %arrayidx8 = getelementptr inbounds %struct.matrix, %struct.matrix* %a, i64 0, i32 0, i64 0, i64 0
  %arrayidx18 = getelementptr inbounds %struct.matrix, %struct.matrix* %c, i64 0, i32 0, i64 0, i64 0
  %0 = load double, double* %arrayidx8
  %arrayidx13 = getelementptr inbounds %struct.matrix, %struct.matrix* %b, i64 0, i32 0, i64 0, i64 0
  %1 = load double, double* %arrayidx13
  %mul = fmul double %0, %1
  %2 = load double, double* %arrayidx18
  %add = fadd double %2, %mul
  store double %add, double* %arrayidx18
  %arrayidx8.1 = getelementptr inbounds %struct.matrix, %struct.matrix* %a, i64 0, i32 0, i64 0, i64 1
  %3 = load double, double* %arrayidx8.1
  %arrayidx13.1 = getelementptr inbounds %struct.matrix, %struct.matrix* %b, i64 0, i32 0, i64 1, i64 0
  %4 = load double, double* %arrayidx13.1
  %mul.1 = fmul double %3, %4
  %add.1 = fadd double %add, %mul.1
  store double %add.1, double* %arrayidx18
  %arrayidx8.2 = getelementptr inbounds %struct.matrix, %struct.matrix* %a, i64 0, i32 0, i64 0, i64 2
  %5 = load double, double* %arrayidx8.2
  %arrayidx13.2 = getelementptr inbounds %struct.matrix, %struct.matrix* %b, i64 0, i32 0, i64 2, i64 0
  %6 = load double, double* %arrayidx13.2
  %mul.2 = fmul double %5, %6
  %add.2 = fadd double %add.1, %mul.2
  store double %add.2, double* %arrayidx18
  %arrayidx18.1 = getelementptr inbounds %struct.matrix, %struct.matrix* %c, i64 0, i32 0, i64 0, i64 1
  %7 = load double, double* %arrayidx8
  %arrayidx13.140 = getelementptr inbounds %struct.matrix, %struct.matrix* %b, i64 0, i32 0, i64 0, i64 1
  %8 = load double, double* %arrayidx13.140
  %mul.141 = fmul double %7, %8
  %9 = load double, double* %arrayidx18.1
  %add.142 = fadd double %9, %mul.141
  store double %add.142, double* %arrayidx18.1
  %10 = load double, double* %arrayidx8.1
  %arrayidx13.1.1 = getelementptr inbounds %struct.matrix, %struct.matrix* %b, i64 0, i32 0, i64 1, i64 1
  %11 = load double, double* %arrayidx13.1.1
  %mul.1.1 = fmul double %10, %11
  %add.1.1 = fadd double %add.142, %mul.1.1
  store double %add.1.1, double* %arrayidx18.1
  %12 = load double, double* %arrayidx8.2
  %arrayidx13.2.1 = getelementptr inbounds %struct.matrix, %struct.matrix* %b, i64 0, i32 0, i64 2, i64 1
  %13 = load double, double* %arrayidx13.2.1
  %mul.2.1 = fmul double %12, %13
  %add.2.1 = fadd double %add.1.1, %mul.2.1
  store double %add.2.1, double* %arrayidx18.1
  %arrayidx18.2 = getelementptr inbounds %struct.matrix, %struct.matrix* %c, i64 0, i32 0, i64 0, i64 2
  %14 = load double, double* %arrayidx8
  %arrayidx13.243 = getelementptr inbounds %struct.matrix, %struct.matrix* %b, i64 0, i32 0, i64 0, i64 2
  %15 = load double, double* %arrayidx13.243
  %mul.244 = fmul double %14, %15
  %16 = load double, double* %arrayidx18.2
  %add.245 = fadd double %16, %mul.244
  store double %add.245, double* %arrayidx18.2
  %17 = load double, double* %arrayidx8.1
  %arrayidx13.1.2 = getelementptr inbounds %struct.matrix, %struct.matrix* %b, i64 0, i32 0, i64 1, i64 2
  %18 = load double, double* %arrayidx13.1.2
  %mul.1.2 = fmul double %17, %18
  %add.1.2 = fadd double %add.245, %mul.1.2
  store double %add.1.2, double* %arrayidx18.2
  %19 = load double, double* %arrayidx8.2
  %arrayidx13.2.2 = getelementptr inbounds %struct.matrix, %struct.matrix* %b, i64 0, i32 0, i64 2, i64 2
  %20 = load double, double* %arrayidx13.2.2
  %mul.2.2 = fmul double %19, %20
  %add.2.2 = fadd double %add.1.2, %mul.2.2
  store double %add.2.2, double* %arrayidx18.2
  %arrayidx8.146 = getelementptr inbounds %struct.matrix, %struct.matrix* %a, i64 0, i32 0, i64 1, i64 0
  %arrayidx18.147 = getelementptr inbounds %struct.matrix, %struct.matrix* %c, i64 0, i32 0, i64 1, i64 0
  %21 = load double, double* %arrayidx8.146
  %22 = load double, double* %arrayidx13
  %mul.149 = fmul double %21, %22
  %23 = load double, double* %arrayidx18.147
  %add.150 = fadd double %23, %mul.149
  store double %add.150, double* %arrayidx18.147
  %arrayidx8.1.151 = getelementptr inbounds %struct.matrix, %struct.matrix* %a, i64 0, i32 0, i64 1, i64 1
  %24 = load double, double* %arrayidx8.1.151
  %25 = load double, double* %arrayidx13.1
  %mul.1.153 = fmul double %24, %25
  %add.1.154 = fadd double %add.150, %mul.1.153
  store double %add.1.154, double* %arrayidx18.147
  %arrayidx8.2.155 = getelementptr inbounds %struct.matrix, %struct.matrix* %a, i64 0, i32 0, i64 1, i64 2
  %26 = load double, double* %arrayidx8.2.155
  %27 = load double, double* %arrayidx13.2
  %mul.2.157 = fmul double %26, %27
  %add.2.158 = fadd double %add.1.154, %mul.2.157
  store double %add.2.158, double* %arrayidx18.147
  %arrayidx18.1.1 = getelementptr inbounds %struct.matrix, %struct.matrix* %c, i64 0, i32 0, i64 1, i64 1
  %28 = load double, double* %arrayidx8.146
  %29 = load double, double* %arrayidx13.140
  %mul.141.1 = fmul double %28, %29
  %30 = load double, double* %arrayidx18.1.1
  %add.142.1 = fadd double %30, %mul.141.1
  store double %add.142.1, double* %arrayidx18.1.1
  %31 = load double, double* %arrayidx8.1.151
  %32 = load double, double* %arrayidx13.1.1
  %mul.1.1.1 = fmul double %31, %32
  %add.1.1.1 = fadd double %add.142.1, %mul.1.1.1
  store double %add.1.1.1, double* %arrayidx18.1.1
  %33 = load double, double* %arrayidx8.2.155
  %34 = load double, double* %arrayidx13.2.1
  %mul.2.1.1 = fmul double %33, %34
  %add.2.1.1 = fadd double %add.1.1.1, %mul.2.1.1
  store double %add.2.1.1, double* %arrayidx18.1.1
  %arrayidx18.2.1 = getelementptr inbounds %struct.matrix, %struct.matrix* %c, i64 0, i32 0, i64 1, i64 2
  %35 = load double, double* %arrayidx8.146
  %36 = load double, double* %arrayidx13.243
  %mul.244.1 = fmul double %35, %36
  %37 = load double, double* %arrayidx18.2.1
  %add.245.1 = fadd double %37, %mul.244.1
  store double %add.245.1, double* %arrayidx18.2.1
  %38 = load double, double* %arrayidx8.1.151
  %39 = load double, double* %arrayidx13.1.2
  %mul.1.2.1 = fmul double %38, %39
  %add.1.2.1 = fadd double %add.245.1, %mul.1.2.1
  store double %add.1.2.1, double* %arrayidx18.2.1
  %40 = load double, double* %arrayidx8.2.155
  %41 = load double, double* %arrayidx13.2.2
  %mul.2.2.1 = fmul double %40, %41
  %add.2.2.1 = fadd double %add.1.2.1, %mul.2.2.1
  store double %add.2.2.1, double* %arrayidx18.2.1
  %arrayidx8.260 = getelementptr inbounds %struct.matrix, %struct.matrix* %a, i64 0, i32 0, i64 2, i64 0
  %arrayidx18.261 = getelementptr inbounds %struct.matrix, %struct.matrix* %c, i64 0, i32 0, i64 2, i64 0
  %42 = load double, double* %arrayidx8.260
  %43 = load double, double* %arrayidx13
  %mul.263 = fmul double %42, %43
  %44 = load double, double* %arrayidx18.261
  %add.264 = fadd double %44, %mul.263
  store double %add.264, double* %arrayidx18.261
  %arrayidx8.1.265 = getelementptr inbounds %struct.matrix, %struct.matrix* %a, i64 0, i32 0, i64 2, i64 1
  %45 = load double, double* %arrayidx8.1.265
  %46 = load double, double* %arrayidx13.1
  %mul.1.267 = fmul double %45, %46
  %add.1.268 = fadd double %add.264, %mul.1.267
  store double %add.1.268, double* %arrayidx18.261
  %arrayidx8.2.269 = getelementptr inbounds %struct.matrix, %struct.matrix* %a, i64 0, i32 0, i64 2, i64 2
  %47 = load double, double* %arrayidx8.2.269
  %48 = load double, double* %arrayidx13.2
  %mul.2.271 = fmul double %47, %48
  %add.2.272 = fadd double %add.1.268, %mul.2.271
  store double %add.2.272, double* %arrayidx18.261
  %arrayidx18.1.2 = getelementptr inbounds %struct.matrix, %struct.matrix* %c, i64 0, i32 0, i64 2, i64 1
  %49 = load double, double* %arrayidx8.260
  %50 = load double, double* %arrayidx13.140
  %mul.141.2 = fmul double %49, %50
  %51 = load double, double* %arrayidx18.1.2
  %add.142.2 = fadd double %51, %mul.141.2
  store double %add.142.2, double* %arrayidx18.1.2
  %52 = load double, double* %arrayidx8.1.265
  %53 = load double, double* %arrayidx13.1.1
  %mul.1.1.2 = fmul double %52, %53
  %add.1.1.2 = fadd double %add.142.2, %mul.1.1.2
  store double %add.1.1.2, double* %arrayidx18.1.2
  %54 = load double, double* %arrayidx8.2.269
  %55 = load double, double* %arrayidx13.2.1
  %mul.2.1.2 = fmul double %54, %55
  %add.2.1.2 = fadd double %add.1.1.2, %mul.2.1.2
  store double %add.2.1.2, double* %arrayidx18.1.2
  %arrayidx18.2.2 = getelementptr inbounds %struct.matrix, %struct.matrix* %c, i64 0, i32 0, i64 2, i64 2
  %56 = load double, double* %arrayidx8.260
  %57 = load double, double* %arrayidx13.243
  %mul.244.2 = fmul double %56, %57
  %58 = load double, double* %arrayidx18.2.2
  %add.245.2 = fadd double %58, %mul.244.2
  store double %add.245.2, double* %arrayidx18.2.2
  %59 = load double, double* %arrayidx8.1.265
  %60 = load double, double* %arrayidx13.1.2
  %mul.1.2.2 = fmul double %59, %60
  %add.1.2.2 = fadd double %add.245.2, %mul.1.2.2
  store double %add.1.2.2, double* %arrayidx18.2.2
  %61 = load double, double* %arrayidx8.2.269
  %62 = load double, double* %arrayidx13.2.2
  %mul.2.2.2 = fmul double %61, %62
  %add.2.2.2 = fadd double %add.1.2.2, %mul.2.2.2
  store double %add.2.2.2, double* %arrayidx18.2.2
  ret void
}
