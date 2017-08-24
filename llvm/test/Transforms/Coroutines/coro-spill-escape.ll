; Check that if alloca escapes it is always put into coroutine frame.
; Even if it is not accessed beyound suspend points.
; RUN: opt < %s -coro-split -S | FileCheck %s

define i8* @escaped.case() "coroutine.presplit"="1" {
entry:
  %sneaky = alloca i32
  %id = call token @llvm.coro.id(i32 0, i8* null, i8* null, i8* null)
  %size = call i32 @llvm.coro.size.i32()
  %alloc = call i8* @malloc(i32 %size)
  %hdl = call i8* @llvm.coro.begin(token %id, i8* %alloc)

  %hideit = ptrtoint i32* %sneaky to i64
  call void @escape(i64 %hideit)

  %tok = call i8 @llvm.coro.suspend(token none, i1 false)
  switch i8 %tok, label %suspend [i8 0, label %resume
                                i8 1, label %cleanup]
resume:
  br label %cleanup

cleanup:
  %mem = call i8* @llvm.coro.free(token %id, i8* %hdl)
  call void @free(i8* %mem)
  br label %suspend
suspend:
  call i1 @llvm.coro.end(i8* %hdl, i1 0)
  ret i8* %hdl
}

; Verify that we spilled alloca due to escape.
; CHECK: %escaped.case.Frame = type { void (%escaped.case.Frame*)*, void (%escaped.case.Frame*)*, i1, i1, i32 }

; CHECK-LABEL: @escaped.case(
; CHECK:   %[[sneaky:.+]] = getelementptr inbounds %escaped.case.Frame, %escaped.case.Frame* %FramePtr, i32 0, i32 4
; CHECK:   %[[hideit:.+]]  = ptrtoint i32* %[[sneaky]] to i64
; CHECK:   call void @escape(i64 %[[hideit]])
; CHECK:   ret i8* %hdl

declare i8* @llvm.coro.free(token, i8*)
declare i32 @llvm.coro.size.i32()
declare i8  @llvm.coro.suspend(token, i1)

declare token @llvm.coro.id(i32, i8*, i8*, i8*)
declare i1 @llvm.coro.alloc(token)
declare i8* @llvm.coro.begin(token, i8*)
declare i1 @llvm.coro.end(i8*, i1)

declare noalias i8* @malloc(i32)
declare void @escape(i64)
declare void @free(i8*)
