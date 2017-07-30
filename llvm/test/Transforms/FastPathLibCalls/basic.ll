; RUN: opt -S < %s -passes='fast-path-lib-calls,verify<domtree>' -fast-path-force-max-op-byte-size=4 -fast-path-force-max-iterations=3 | FileCheck %s

define void @baseline(i8* %ptr, i64 %size) {
; CHECK-LABEL: define void @baseline(
entry:
  call void @llvm.memset.p0i8.i64(i8* %ptr, i8 15, i64 %size, i32 1, i1 false)
  ret void
; CHECK:       entry:
; CHECK-NEXT:    %[[ZERO_COND:.*]] = icmp ne i64 %size, 0
; CHECK-NEXT:    br i1 %[[ZERO_COND]], label %[[IF:.*]], label %[[TAIL:.*]]
;
; CHECK:       [[IF]]:
; CHECK-NEXT:    %[[COUNT_COND:.*]] = icmp ule i64 %size, 3
; CHECK-NEXT:    br i1 %[[COUNT_COND]], label %[[THEN:.*]], label %[[ELSE:.*]]
;
; CHECK:       [[THEN]]:
; CHECK-NEXT:    %[[INDEX:.*]] = phi i64 [ 0, %[[IF]] ], [ %[[NEXT_INDEX:.*]], %[[THEN]] ]
; CHECK-NEXT:    %[[INDEXED_DST:.*]] = getelementptr inbounds i8, i8* %ptr, i64 %[[INDEX]]
; CHECK-NEXT:    store i8 15, i8* %[[INDEXED_DST]], align 1
; CHECK-NEXT:    %[[NEXT_INDEX]] = add i64 %[[INDEX]], 1
; CHECK-NEXT:    %[[LOOP_COND:.*]] = icmp eq i64 %[[NEXT_INDEX]], %size
; CHECK-NEXT:    br i1 %[[LOOP_COND]], label %[[TAIL:.*]], label %[[THEN]]
;
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    call void @llvm.memset.p0i8.i64(i8* %ptr, i8 15, i64 %size, i32 1, i1 false)
; CHECK-NEXT:    br label %[[TAIL]]
;
; CHECK:       [[TAIL]]:
; CHECK-NEXT:    ret void
}

; Negative tests where we shouldn't do anything.

define void @optsize(i8* %ptr, i64 %size) optsize {
; CHECK-LABEL: define void @optsize(
entry:
  call void @llvm.memset.p0i8.i64(i8* %ptr, i8 15, i64 %size, i32 1, i1 false)
  ret void
; CHECK:       entry:
; CHECK-NEXT:    call void @llvm.memset.p0i8.i64(i8* %ptr, i8 15, i64 %size, i32 1, i1 false)
; CHECK-NEXT:    ret void
}

define void @minsize(i8* %ptr, i64 %size) minsize {
; CHECK-LABEL: define void @minsize(
entry:
  call void @llvm.memset.p0i8.i64(i8* %ptr, i8 15, i64 %size, i32 1, i1 false)
  ret void
; CHECK:       entry:
; CHECK-NEXT:    call void @llvm.memset.p0i8.i64(i8* %ptr, i8 15, i64 %size, i32 1, i1 false)
; CHECK-NEXT:    ret void
}

declare void @llvm.memset.p0i8.i64(i8* writeonly, i8, i64, i32, i1)
