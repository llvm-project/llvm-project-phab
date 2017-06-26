; RUN: opt < %s -sroa -S | FileCheck %s
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux"



%foo = type { %bar* }
%bar = type { [4 x i8] }

declare void @foo(i64* noalias nocapture)

; Make sure we properly handle the !nonnull attribute when we
; convert a pointer load to an integer load.
; CHECK-LABEL: @_ZN4core3fmt9Formatter12pad_integral17h1dcf0f409406b6e5E()
; CHECK-NEXT: start:
; CHECK-NEXT: %0 = inttoptr i64 0 to %bar*
; CHECK-NEXT: ret %bar* %0
define %bar* @_ZN4core3fmt9Formatter12pad_integral17h1dcf0f409406b6e5E()  {
start:
  %arg4.i = alloca %foo, align 8

  %0 = bitcast %foo* %arg4.i to i64*
  store i64 0, i64* %0, align 8

  %1 = getelementptr inbounds %foo, %foo* %arg4.i, i64 0, i32 0
  %2 = load %bar*, %bar** %1, align 8, !nonnull !0
  ret %bar* %2
}

!0 = !{}
