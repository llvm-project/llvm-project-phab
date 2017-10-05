; RUN: opt < %s -partial-inliner -S  | FileCheck %s
; RUN: opt < %s -passes=partial-inliner -S  | FileCheck %s

%"class.base" = type { %"struct.base"* }
%"struct.base" = type opaque

@g = external local_unnamed_addr global i32, align 4
@status = external local_unnamed_addr global i32, align 4

define i32 @vararg(i32 %count, ...) {
bb:
  %tmp = alloca  %"class.base", align 4
  %status_loaded = load i32, i32* @status, align 4
  %tmp4 = icmp slt i32 %status_loaded, 0
  br i1 %tmp4, label %bb6, label %bb5

bb5:                                              ; preds = %bb
  %tmp11 = bitcast %"class.base"* %tmp to i32*
  %tmp2 = load i32, i32* @g, align 4
  %tmp3 = add nsw i32 %tmp2, 1
  store i32 %tmp3, i32* %tmp11, align 4
  store i32 %tmp3, i32* @g, align 4
  call void @bar(i32 %count, i32* nonnull %tmp11) #2
  br label %bb6

bb6:                                              ; preds = %bb5, %bb
  %tmp7 = phi i32 [ 1, %bb5 ], [ 0, %bb ]
  ret i32 %tmp7
}

declare void @bar(i32, i32*)

define i32 @caller(i32 %arg) local_unnamed_addr {
bb:
  %tmp = tail call i32 (i32, ...) @vararg(i32 %arg)
  ret i32 %tmp
}

; CHECK-LABEL: @caller
; CHECK: codeRepl.i:
; CHECK-NEXT:  call void (%class.base*, i32, ...) @vararg.1_bb5(%class.base* %tmp.i, i32 %arg)
