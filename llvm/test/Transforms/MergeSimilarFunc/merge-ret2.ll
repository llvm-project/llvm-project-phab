; Check that two ret instructions with the same return type
; are generated in the merged function. A bug caused a mismatch
; in return types here and a verifier failure.
;
; RUN: opt -S -mergesimilarfunc < %s | FileCheck %s
;
; CHECK: @mrg1__merged
; CHECK: switch i32 %__merge_arg
; CHECK: ret %0*
; CHECK: ret %0*

target datalayout = "e-m:e-p:32:32:32-a:0-n16:32-i64:64:64-i32:32:32-i16:16:16-i1:8:8-f32:32:32-f64:64:64-v32:32:32-v64:64:64-v512:512:512-v1024:1024:1024-v2048:2048:2048"

%0 = type { {}*, %0* (%8*)*, %0* (%13*)*, %0* (%39*)*, %0* (%14*)*, %0* (%14*)*, %0* (%15*)*, %0* (%16*)*, %0* ()*, %0* (%17*)*, %0* (%22*)*, %0* (%28*)*, %0* (%32*)*, %0* (%37*)*, %0* (%37*)*, %0* (%37*)*, %0* ()*, %0* (%33*)*, %0* (%36*)*, %0* (%37*)*, %0* (%38*)*, %0* ()*, %0* (%39*)*, %0* ()*, %0* ()*, %0* (%40*)*, %0* ()*, %0* ()*, %0* ()*, %0* ()*, i8 ()*, void ()* }
%1 = type { %2, i32, i8, i8, i64, i8, i8, i8, %3, %3, i8, i8 }
%2 = type { i8, i8, i8, i8 }
%3 = type { i32, i32, [10 x %4], %7 }
%4 = type { %5, i8, i32, i8, %5 }
%5 = type { %6 }
%6 = type { i64, i64, i64, i64 }
%7 = type { i8, i8 }
%8 = type { %2, i32, i8, i8, i8, i8, i8, i64, %9, %3, i8, i8, i8, %12, i32, i8 }
%9 = type { %10 }
%10 = type { i8, i8, %11, i32, i8 }
%11 = type { [3 x i8] }
%12 = type { i8, i8, i8 }
%13 = type { %2, i32, i8, i32, i8, i8, i8 }
%14 = type { %2, i32, i8, i8, i8 }
%15 = type { %2, i8, i8, i8 }
%16 = type { %2, i8, %5, i8, %11, i16, i8 }
%17 = type { %2, %18, i8 }
%18 = type { i8, i8, i8, i8, i8, i8, i8, i8, %11, %19, %19, %20, i32, %21, i8, i16, i8 }
%19 = type { i8, i8, i8, [255 x i8], i8 }
%20 = type { i8, i8, i8, i8, i8, i8, i32 }
%21 = type { i8, [3 x i8] }
%22 = type { %2, i8, i8, %23, i8, i8, i8 }
%23 = type { %24 }
%24 = type { i32, [40 x %25] }
%25 = type { %11, i8, i8, i8, i8, i8, i32, %26, i32 }
%26 = type { i32, %27, i8 }
%27 = type { i8, [48 x i8] }
%28 = type { %2, i8, %29, i8, %24, i8, %31, i8, i8, i8, i8, i16, i8 }
%29 = type { i8, %11, i8, i8, i16, i8, i8, i8, %5, i16, i16, i8, %26, i8, i8, i8, i32, i8, i8, i8, i8, %30, i8, i8, %11, i8 }
%30 = type { i8, i8, i8, i8, i8 }
%31 = type { i32, [40 x %11] }
%32 = type { %2, i8, %29, i8, i8, %31, i8, i16, i8 }
%33 = type { %2, %34, i8 }
%34 = type { i8, [16 x %35] }
%35 = type { i8, i8, [41 x i8] }
%36 = type { %2, i32, i8 }
%37 = type { %2, i8 }
%38 = type { %2, i8, i8, %11, i16, i8, i32, i8 }
%39 = type { %2, i32, i8, i8 }
%40 = type { %2, i8, %11, i32, i8, i8, i8 }
%41 = type { {}*, %41* (%8*)*, %41* (%13*)*, %41* (%39*)*, %41* (%14*)*, %41* (%14*)*, %41* (%15*)*, %41* (%16*)*, %41* ()*, %41* (%17*)*, %41* (%22*)*, %41* (%28*)*, %41* (%32*)*, %41* (%37*)*, %41* (%37*)*, %41* (%37*)*, %41* ()*, %41* (%33*)*, %41* (%36*)*, %41* (%37*)*, %41* (%38*)*, %41* ()*, %41* (%39*)*, %41* ()*, %41* ()*, %41* (%40*)*, %41* ()*, %41* ()*, %41* ()*, %41* ()*, i8 ()*, void ()* }

@xx = external global i8, align 1
@yy = external global { %0* (%1*)*, %0* (%8*)*, %0* (%13*)*, %0* (%39*)*, %0* (%14*)*, %0* (%14*)*, %0* (%15*)*, %0* (%16*)*, %0* ()*, %0* (%17*)*, %0* (%22*)*, %0* (%28*)*, %0* (%32*)*, %0* (%37*)*, %0* (%37*)*, %0* (%37*)*, %0* ()*, %0* (%33*)*, %0* (%36*)*, %0* (%37*)*, %0* (%38*)*, %0* ()*, %0* (%39*)*, %0* ()*, %0* ()*, %0* (%40*)*, %0* ()*, %0* ()*, %0* ()*, %0* ()*, i8 ()*, void ()* }, align 4
@zz = external global { %41* (%1*)*, %41* (%8*)*, %41* (%13*)*, %41* (%39*)*, %41* (%14*)*, %41* (%14*)*, %41* (%15*)*, %41* (%16*)*, %41* ()*, %41* (%17*)*, %41* (%22*)*, %41* (%28*)*, %41* (%32*)*, %41* (%37*)*, %41* (%37*)*, %41* (%37*)*, %41* ()*, %41* (%33*)*, %41* (%36*)*, %41* (%37*)*, %41* (%38*)*, %41* ()*, %41* (%39*)*, %41* ()*, %41* ()*, %41* (%40*)*, %41* ()*, %41* ()*, %41* ()*, %41* ()*, i8 ()*, void ()* }, align 4

declare void @llvm.lifetime.start(i64, i8* nocapture)
declare void @llvm.lifetime.end(i64, i8* nocapture)
declare void @f1()
declare void @f1a(i8 zeroext)
declare signext i8 @f2()
declare void @f3(%26* noalias nocapture sret)

define internal %0* @mrg1() optsize {
  %1 = alloca %26, align 4
  %2 = bitcast %26* %1 to i8*
  call void @llvm.lifetime.start(i64 56, i8* %2)
  call void @f3(%26* nonnull sret %1)
  call void @f1a(i8 zeroext 1)
  %3 = load i8, i8* @xx, align 1
  %4 = icmp eq i8 %3, 1
  br i1 %4, label %5, label %18

; <label>:5:                                      ; preds = %0
  %6 = call signext i8 @f2()
  %7 = icmp eq i8 %6, 2
  br i1 %7, label %8, label %18

; <label>:8:                                      ; preds = %5
  %9 = getelementptr inbounds %26, %26* %1, i32 0, i32 0
  %10 = load i32, i32* %9, align 4
  %11 = icmp eq i32 %10, -1
  br i1 %11, label %16, label %12

; <label>:12:                                     ; preds = %8
  %13 = getelementptr inbounds %26, %26* %1, i32 0, i32 2
  %14 = load i8, i8* %13, align 1
  %15 = icmp eq i8 %14, 1
  br i1 %15, label %16, label %17

; <label>:16:                                     ; preds = %12, %8
  call void @f1()
  br label %17

; <label>:17:                                     ; preds = %16, %12
  store i8 0, i8* @xx, align 1
  br label %18

; <label>:18:                                     ; preds = %17, %5, %0
  call void @llvm.lifetime.end(i64 56, i8* %2)
  ret %0* bitcast ({ %0* (%1*)*, %0* (%8*)*, %0* (%13*)*, %0* (%39*)*, %0* (%14*)*, %0* (%14*)*, %0* (%15*)*, %0* (%16*)*, %0* ()*, %0* (%17*)*, %0* (%22*)*, %0* (%28*)*, %0* (%32*)*, %0* (%37*)*, %0* (%37*)*, %0* (%37*)*, %0* ()*, %0* (%33*)*, %0* (%36*)*, %0* (%37*)*, %0* (%38*)*, %0* ()*, %0* (%39*)*, %0* ()*, %0* ()*, %0* (%40*)*, %0* ()*, %0* ()*, %0* ()*, %0* ()*, i8 ()*, void ()* }* @yy to %0*)
}

define internal %41* @mrg2() optsize {
  %1 = alloca %26, align 4
  %2 = bitcast %26* %1 to i8*
  call void @llvm.lifetime.start(i64 56, i8* %2)
  call void @f3(%26* nonnull sret %1)
  call void @f1a(i8 zeroext 1)
  %3 = load i8, i8* @xx, align 1
  %4 = icmp eq i8 %3, 1
  br i1 %4, label %5, label %18

; <label>:5:                                      ; preds = %0
  %6 = call signext i8 @f2()
  %7 = icmp eq i8 %6, 2
  br i1 %7, label %8, label %18

; <label>:8:                                      ; preds = %5
  %9 = getelementptr inbounds %26, %26* %1, i32 0, i32 0
  %10 = load i32, i32* %9, align 4
  %11 = icmp eq i32 %10, -1
  br i1 %11, label %16, label %12

; <label>:12:                                     ; preds = %8
  %13 = getelementptr inbounds %26, %26* %1, i32 0, i32 2
  %14 = load i8, i8* %13, align 1
  %15 = icmp eq i8 %14, 1
  br i1 %15, label %16, label %17

; <label>:16:                                     ; preds = %12, %8
  call void @f1()
  br label %17

; <label>:17:                                     ; preds = %16, %12
  store i8 0, i8* @xx, align 1
  br label %18

; <label>:18:                                     ; preds = %17, %5, %0
  call void @llvm.lifetime.end(i64 56, i8* %2)
  ret %41* bitcast ({ %41* (%1*)*, %41* (%8*)*, %41* (%13*)*, %41* (%39*)*, %41* (%14*)*, %41* (%14*)*, %41* (%15*)*, %41* (%16*)*, %41* ()*, %41* (%17*)*, %41* (%22*)*, %41* (%28*)*, %41* (%32*)*, %41* (%37*)*, %41* (%37*)*, %41* (%37*)*, %41* ()*, %41* (%33*)*, %41* (%36*)*, %41* (%37*)*, %41* (%38*)*, %41* ()*, %41* (%39*)*, %41* ()*, %41* ()*, %41* (%40*)*, %41* ()*, %41* ()*, %41* ()*, %41* ()*, i8 ()*, void ()* }* @zz to %41*)
}

