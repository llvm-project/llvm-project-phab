; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -instcombine -licm -S | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: norecurse nounwind readonly uwtable
define i32 @foo(i8* nocapture readnone %match, i32 %cur_match, i32 %best_len, i32 %scan_end, i32* nocapture readonly %prev, i32 %limit, i32 %chain_length, i8* nocapture readonly %win, i32 %wmask) local_unnamed_addr #0 {
; CHECK-LABEL: @foo(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[IDX_EXT2:%.*]] = zext i32 [[CUR_MATCH:%.*]] to i64
; CHECK-NEXT:    [[ADD_PTR4:%.*]] = getelementptr inbounds i8, i8* [[WIN:%.*]], i64 [[IDX_EXT2]]
; CHECK-NEXT:    [[IDX_EXT1:%.*]] = zext i32 [[BEST_LEN:%.*]] to i64
; CHECK:       if.then.lr.ph:
; CHECK-NEXT:    [[ADD_PTR2_SUM:%.*]] = add nsw i64 [[IDX_EXT1]], -1
; CHECK-NEXT:    br label [[IF_THEN:%.*]]
; CHECK:       do.body:
; CHECK-NEXT:    [[IDX_EXT:%.*]] = zext i32 [[TMP4:%.*]] to i64
; CHECK-NEXT:    [[ADD_PTR:%.*]] = getelementptr inbounds i8, i8* [[WIN]], i64 [[IDX_EXT]]
; CHECK-NEXT:    [[ADD_PTR3:%.*]] = getelementptr inbounds i8, i8* [[ADD_PTR]], i64 [[ADD_PTR2_SUM]]
;
entry:
  %idx.ext2 = zext i32 %cur_match to i64
  %add.ptr4 = getelementptr inbounds i8, i8* %win, i64 %idx.ext2
  %idx.ext1 = zext i32 %best_len to i64
  %add.ptr25 = getelementptr inbounds i8, i8* %add.ptr4, i64 %idx.ext1
  %add.ptr36 = getelementptr inbounds i8, i8* %add.ptr25, i64 -1
  %0 = bitcast i8* %add.ptr36 to i32*
  %1 = load i32, i32* %0, align 4, !tbaa !2
  %cmp7 = icmp eq i32 %1, %scan_end
  br i1 %cmp7, label %do.end, label %if.then.lr.ph

if.then.lr.ph:                                    ; preds = %entry
  br label %if.then

do.body:                                          ; preds = %land.lhs.true
  %chain_length.addr.0 = phi i32 [ %dec, %land.lhs.true ]
  %cur_match.addr.0 = phi i32 [ %4, %land.lhs.true ]
  %idx.ext = zext i32 %cur_match.addr.0 to i64
  %add.ptr = getelementptr inbounds i8, i8* %win, i64 %idx.ext
  %add.ptr2 = getelementptr inbounds i8, i8* %add.ptr, i64 %idx.ext1
  %add.ptr3 = getelementptr inbounds i8, i8* %add.ptr2, i64 -1
  %2 = bitcast i8* %add.ptr3 to i32*
  %3 = load i32, i32* %2, align 4, !tbaa !2
  %cmp = icmp eq i32 %3, %scan_end
  br i1 %cmp, label %do.end, label %if.then

if.then:                                          ; preds = %if.then.lr.ph, %do.body
  %cur_match.addr.09 = phi i32 [ %cur_match, %if.then.lr.ph ], [ %cur_match.addr.0, %do.body ]
  %chain_length.addr.08 = phi i32 [ %chain_length, %if.then.lr.ph ], [ %chain_length.addr.0, %do.body ]
  %and = and i32 %cur_match.addr.09, %wmask
  %idxprom = zext i32 %and to i64
  %arrayidx = getelementptr inbounds i32, i32* %prev, i64 %idxprom
  %4 = load i32, i32* %arrayidx, align 4, !tbaa !2
  %cmp4 = icmp ugt i32 %4, %limit
  br i1 %cmp4, label %land.lhs.true, label %do.end

land.lhs.true:                                    ; preds = %if.then
  %dec = add i32 %chain_length.addr.08, -1
  %cmp5 = icmp eq i32 %dec, 0
  br i1 %cmp5, label %do.end, label %do.body

do.end:                                           ; preds = %do.body, %land.lhs.true, %if.then, %entry
  %cont.0 = phi i32 [ 1, %entry ], [ 0, %if.then ], [ 0, %land.lhs.true ], [ 1, %do.body ]
  ret i32 %cont.0
}

attributes #0 = { norecurse nounwind readonly uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0 (https://github.com/llvm-mirror/clang.git ba0893fc83cd0f03803ca0799d7b6f10bc13d128) (https://github.com/llvm-mirror/llvm.git 0abad4677f86840863c02c575fb078055c1cb114)"}
!2 = !{!3, !3, i64 0}
!3 = !{!"int", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
