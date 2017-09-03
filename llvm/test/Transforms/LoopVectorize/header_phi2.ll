;RUN: opt < %s  -loop-vectorize  -S | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-grtev4-linux-gnu"

define  void @foo() {
;CHECK-LABEL: @foo
;CHECK: vector.body:
;CHECK: store <4 x i32>
bb:
  br label %bb56

bb56:                                             ; preds = %bb100, %bb
  %tmp = phi i32* [ undef, %bb ], [ %tmp98, %bb100 ]
  br label %bb67

bb67:                                             ; preds = %bb56
  br i1 undef, label %bb68, label %bb97

bb68:                                             ; preds = %bb67
  br label %bb74

bb73:                                             ; preds = %bb92
  unreachable

bb74:                                             ; preds = %bb68
  br label %bb75

bb75:                                             ; preds = %bb74
  br i1 undef, label %bb76, label %bb77

bb76:                                             ; preds = %bb75
  br label %bb97

bb77:                                             ; preds = %bb75
  br label %bb83

bb83:                                             ; preds = %bb92, %bb77
  %tmp84 = phi i32* [ %tmp93, %bb92 ], [ undef, %bb77 ]
  br label %bb85

bb85:                                             ; preds = %bb85, %bb83
  %tmp86 = phi i32* [ %tmp88, %bb85 ], [ %tmp84, %bb83 ]
  %tmp87 = phi i32* [ %tmp90, %bb85 ], [ %tmp, %bb83 ]
  %tmp88 = getelementptr inbounds i32, i32* %tmp86, i64 1
  store i32 undef, i32* %tmp86, align 4
  %tmp89 = load i32, i32* %tmp88, align 4
  %tmp90 = getelementptr inbounds i32, i32* %tmp87, i64 1
  store i32 %tmp89, i32* %tmp87, align 4
  %tmp91 = icmp ugt i32* undef, %tmp90
  br i1 %tmp91, label %bb85, label %bb92

bb92:                                             ; preds = %bb85
  %tmp93 = getelementptr inbounds i32, i32* undef, i64 2
  %tmp94 = icmp sgt i32 undef, undef
  br i1 %tmp94, label %bb83, label %bb73

bb97:                                             ; preds = %bb76, %bb67
  %tmp98 = phi i32* [ undef, %bb76 ], [ %tmp, %bb67 ]
  br label %bb100

bb100:                                            ; preds = %bb97
  br i1 undef, label %bb101, label %bb56

bb101:                                            ; preds = %bb100
  unreachable
}

