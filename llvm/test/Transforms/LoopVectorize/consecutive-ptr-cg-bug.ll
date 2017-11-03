; RUN: opt -loop-vectorize -S < %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128-ni:1"
target triple = "x86_64-unknown-linux-gnu"

; During LV code generation, after introducing middle and scalar.ph basic
; blocks, we expected Legal->isConsecutivePtr() to be consistent and return the
; same output as during legal/cost model phases. Unfortunately, there were some
; corner cases where that didn't happen due to a limitation in SE/SLEV. This
; test verifies that LV is able to handle those corner cases.

; PR34965

; Verify that store is vectorized as stride-1 memory access.

; CHECK: vector.body:
; CHECK: store <4 x i32>

; Function Attrs: uwtable
define void @test() {
  br label %.outer

; <label>:1:                                      ; preds = %2
  ret void

; <label>:2:                                      ; preds = %._crit_edge.loopexit
  %3 = add nsw i32 %.ph, -2
  br i1 undef, label %1, label %.outer

.outer:                                           ; preds = %2, %0
  %.ph = phi i32 [ %3, %2 ], [ 336, %0 ]
  %.ph2 = phi i32 [ 62, %2 ], [ 110, %0 ]
  %4 = and i32 %.ph, 30
  %5 = add i32 %.ph2, 1
  br label %6

; <label>:6:                                      ; preds = %6, %.outer
  %7 = phi i32 [ %5, %.outer ], [ %13, %6 ]
  %8 = phi i32 [ %.ph2, %.outer ], [ %7, %6 ]
  %9 = add i32 %8, 2
  %10 = zext i32 %9 to i64
  %11 = getelementptr inbounds i32, i32 addrspace(1)* undef, i64 %10
  %12 = ashr i32 undef, %4
  store i32 %12, i32 addrspace(1)* %11, align 4
  %13 = add i32 %7, 1
  %14 = icmp sgt i32 %13, 61
  br i1 %14, label %._crit_edge.loopexit, label %6

._crit_edge.loopexit:                             ; preds = %._crit_edge.loopexit, %6
  br i1 undef, label %2, label %._crit_edge.loopexit
}

