; RUN: opt < %s -mem2reg -S | FileCheck %s

target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"

declare void @llvm.memcpy.p0i128.p0i64.i32(i128 *, i64 *, i32, i32, i1)
declare void @llvm.memcpy.p0i64.p0i64.i32(i64 *, i64 *, i32, i32, i1)
declare void @llvm.memcpy.p0f64.p0i64.i32(double *, i64 *, i32, i32, i1)

define i128 @test_cpy_different(i64) {
; CHECK-LABEL: @test_cpy_different
; CHECK-NOT: alloca i64
; CHECK: store i64 %0
    %a = alloca i64
    %b = alloca i128
    store i128 0, i128 *%b
    store i64 %0, i64 *%a
    call void @llvm.memcpy.p0i128.p0i64.i32(i128 *%b, i64 *%a, i32 8, i32 0, i1 0)
    %loaded = load i128, i128 *%b
    ret i128 %loaded
}

define i64 @test_cpy_same(i64) {
; CHECK-LABEL: @test_cpy_same
; CHECK-NOT: alloca
; CHECK: ret i64 %0
    %a = alloca i64
    %b = alloca i64
    store i64 %0, i64 *%a
    call void @llvm.memcpy.p0i64.p0i64.i32(i64 *%b, i64 *%a, i32 8, i32 0, i1 0)
    %loaded = load i64, i64 *%b
    ret i64 %loaded
}

define double @test_cpy_different_type(i64) {
; CHECK-LABEL: @test_cpy_different_type
; CHECK-NOT: alloca
; CHECK: bitcast i64 %0 to double
    %a = alloca i64
    %b = alloca double
    store i64 %0, i64 *%a
    call void @llvm.memcpy.p0f64.p0i64.i32(double *%b, i64 *%a, i32 8, i32 0, i1 0)
    %loaded = load double, double *%b
    ret double %loaded
}
