; RUN: opt < %s -memcpyopt -S | FileCheck %s
; Make sure memcpy-memcpy dependence is optimized across
; basic blocks (conditional branches and invokes).

; ModuleID = 'nonlocal-memcpy-memcpy.cpp'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.11.0"

%struct.s = type { i32, i32 }

@foo_s = private unnamed_addr constant %struct.s { i32 1, i32 2 }, align 4
@baz_s = private unnamed_addr constant %struct.s { i32 1, i32 2 }, align 4
@i = external constant i8*

; Function Attrs: ssp uwtable
define i32 @foo() #0 {
  %x = alloca i32, align 4
  %s = alloca %struct.s, align 4
  %t = alloca %struct.s, align 4
  %1 = call i32 @bar()
  store i32 %1, i32* %x, align 4
  %2 = bitcast %struct.s* %s to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %2, i8* bitcast (%struct.s* @foo_s to i8*), i64 8, i32 4, i1 false)
  %3 = load i32, i32* %x, align 4
  %4 = icmp ne i32 %3, 0
  br i1 %4, label %5, label %8

; <label>:5                                       ; preds = %0
  %6 = bitcast %struct.s* %t to i8*
  %7 = bitcast %struct.s* %s to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %6, i8* %7, i64 8, i32 4, i1 false)

; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* %6, i8* bitcast (%struct.s* @foo_s to i8*), i64 8, i32 4, i1 false)

  br label %8

; <label>:8                                       ; preds = %5, %0
  %9 = getelementptr inbounds %struct.s, %struct.s* %t, i32 0, i32 0
  %10 = load i32, i32* %9, align 4
  %11 = getelementptr inbounds %struct.s, %struct.s* %t, i32 0, i32 1
  %12 = load i32, i32* %11, align 4
  %13 = add nsw i32 %10, %12
  ret i32 %13
}

declare i32 @bar() #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #2

; Function Attrs: ssp uwtable
define i32 @baz() #0 personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
  %x = alloca i32, align 4
  %s = alloca %struct.s, align 4
  %t = alloca %struct.s, align 4
  %1 = alloca i8*
  %2 = alloca i32
  %3 = call i32 @bar()
  store i32 %3, i32* %x, align 4
  %4 = bitcast %struct.s* %s to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %4, i8* bitcast (%struct.s* @baz_s to i8*), i64 8, i32 4, i1 false)
  %5 = load i32, i32* %x, align 4
  %6 = icmp ne i32 %5, 0
  br i1 %6, label %7, label %23

; <label>:7                                       ; preds = %0
  %8 = call i8* @__cxa_allocate_exception(i64 4) #3
  %9 = bitcast i8* %8 to i32*
  store i32 0, i32* %9, align 8
  invoke void @__cxa_throw(i8* %8, i8* bitcast (i8** @i to i8*), i8* null) #4
          to label %26 unwind label %10

; <label>:10                                      ; preds = %7
  %11 = landingpad { i8*, i32 }
          catch i8* null
  %12 = extractvalue { i8*, i32 } %11, 0
  store i8* %12, i8** %1, align 8
  %13 = extractvalue { i8*, i32 } %11, 1
  store i32 %13, i32* %2, align 4
  br label %14

; <label>:14                                      ; preds = %10
  %15 = load i8*, i8** %1, align 8
  %16 = call i8* @__cxa_begin_catch(i8* %15) #3
  call void @__cxa_end_catch()
  br label %17

; <label>:17                                      ; preds = %14, %23
  %18 = getelementptr inbounds %struct.s, %struct.s* %t, i32 0, i32 0
  %19 = load i32, i32* %18, align 4
  %20 = getelementptr inbounds %struct.s, %struct.s* %t, i32 0, i32 1
  %21 = load i32, i32* %20, align 4
  %22 = add nsw i32 %19, %21
  ret i32 %22

; <label>:23                                      ; preds = %0
  %24 = bitcast %struct.s* %t to i8*
  %25 = bitcast %struct.s* %s to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %24, i8* %25, i64 8, i32 4, i1 false)

; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* %24, i8* bitcast (%struct.s* @baz_s to i8*), i64 8, i32 4, i1 false)

  br label %17

; <label>:26                                      ; preds = %7
  unreachable
}

declare i8* @__cxa_allocate_exception(i64)

declare void @__cxa_throw(i8*, i8*, i8*)

declare i32 @__gxx_personality_v0(...)

declare i8* @__cxa_begin_catch(i8*)

declare void @__cxa_end_catch()

