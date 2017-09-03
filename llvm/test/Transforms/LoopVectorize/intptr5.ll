; RUN: opt < %s  -loop-vectorize  -S | FileCheck %s

;CHECK-LABEL: @foo
;CHECK: vector.body:
;CHECK: load <16 x i8>
;CHECK: store <16 x i8>

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-grtev4-linux-gnu"

define void @foo(i64 %tmp0) {
bb:
  br label %bb3

bb3:                                              ; preds = %bb20, %bb
  %tmp = phi i64 [ %tmp0, %bb ], [ %tmp21, %bb20 ]
  br i1 false, label %bb4, label %bb19

bb4:                                              ; preds = %bb3
  %tmp5 = inttoptr i64 %tmp to i8*
  br label %bb7

bb7:                                              ; preds = %bb7, %bb4
  %tmp8 = phi i64 [ %tmp, %bb4 ], [ %tmp16, %bb7 ]
  %tmp9 = phi i8* [ %tmp5, %bb4 ], [ %tmp15, %bb7 ]
  %tmp10 = phi i8* [ undef, %bb4 ], [ %tmp13, %bb7 ]
  %tmp11 = phi i32 [ 0, %bb4 ], [ %tmp17, %bb7 ]
  %tmp12 = load i8, i8* %tmp9, align 1, !range !1, !noalias !2
  %tmp13 = getelementptr inbounds i8, i8* %tmp10, i64 1
  store i8 %tmp12, i8* %tmp10, align 1, !noalias !2
  %tmp14 = inttoptr i64 %tmp8 to i8*
  %tmp15 = getelementptr inbounds i8, i8* %tmp14, i64 1
  %tmp16 = ptrtoint i8* %tmp15 to i64
  %tmp17 = add nuw nsw i32 %tmp11, 1
  %tmp18 = icmp eq i32 %tmp17, undef
  br i1 %tmp18, label %bb19, label %bb7

bb19: 
  br i1 undef, label %bb20, label %bb22

bb20:                                             ; preds = %bb19
  %tmp21 = ptrtoint i8* undef to i64
  br label %bb3

bb22:                                             ; preds = %bb19
  ret void
}

!llvm.ident = !{!0}

!0 = !{!"clang version google3-trunk (trunk r311977)"}
!1 = !{i8 0, i8 2}
!2 = !{!3}
!3 = distinct !{!3, !4, !"_ZN15quality_ranklab4impl12UnionApplierINS_8internal13CondOpWrapperINS2_10UnionSumOpIbEEEEbE10MakeNStepsINS_15ForwardIteratorINS_10FullVectorIbEEEENS_25MaybeFullPropertyInserterIbPbSt20back_insert_iteratorISt6vectorIiSaIiEEEEEEET0_PT_SL_S6_i: argument 0"}
!4 = distinct !{!4, !"_ZN15quality_ranklab4impl12UnionApplierINS_8internal13CondOpWrapperINS2_10UnionSumOpIbEEEEbE10MakeNStepsINS_15ForwardIteratorINS_10FullVectorIbEEEENS_25MaybeFullPropertyInserterIbPbSt20back_insert_iteratorISt6vectorIiSaIiEEEEEEET0_PT_SL_S6_i"}
