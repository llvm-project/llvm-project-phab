; RUN: opt -S < %s -mtriple=x86_64-unknown-linux-gnu -passes=fast-path-lib-calls | FileCheck %s

define void @set1(i8* %ptr, i64 %size) {
; CHECK-LABEL: define void @set1(
entry:
  call void @llvm.memset.p0i8.i64(i8* %ptr, i8 15, i64 %size, i32 1, i1 false)
  ret void
; CHECK:       entry:
; CHECK-NEXT:    %[[ZERO_COND:.*]] = icmp ne i64 %size, 0
; CHECK-NEXT:    br i1 %[[ZERO_COND]], label %[[IF:.*]], label %[[TAIL:.*]]
;
; CHECK:       [[IF]]:
; CHECK-NEXT:    %[[COUNT_COND:.*]] = icmp ule i64 %size, 6
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

define void @set2(i16* %ptr, i64 %size) {
; CHECK-LABEL: define void @set2(
entry:
  %ptr.i8 = bitcast i16* %ptr to i8*
  %size.scaled = shl nuw nsw i64 %size, 1
  call void @llvm.memset.p0i8.i64(i8* %ptr.i8, i8 15, i64 %size.scaled, i32 2, i1 false)
  ret void
; CHECK:       entry:
; CHECK-NEXT:    %[[PTR_I8:.*]] = bitcast i16* %ptr to i8*
; CHECK-NEXT:    %[[ZERO_COND:.*]] = icmp ne i64 %size, 0
; CHECK-NEXT:    br i1 %[[ZERO_COND]], label %[[IF:.*]], label %[[TAIL:.*]]
;
; CHECK:       [[IF]]:
; CHECK-NEXT:    %[[COUNT_COND:.*]] = icmp ule i64 %size, 6
; CHECK-NEXT:    br i1 %[[COUNT_COND]], label %[[THEN:.*]], label %[[ELSE:.*]]
;
; CHECK:       [[THEN]]:
; CHECK-NEXT:    %[[INDEX:.*]] = phi i64 [ 0, %[[IF]] ], [ %[[NEXT_INDEX:.*]], %[[THEN]] ]
; CHECK-NEXT:    %[[INDEXED_DST:.*]] = getelementptr inbounds i16, i16* %ptr, i64 %[[INDEX]]
; CHECK-NEXT:    store i16 3855, i16* %[[INDEXED_DST]], align 2
; CHECK-NEXT:    %[[NEXT_INDEX]] = add i64 %[[INDEX]], 1
; CHECK-NEXT:    %[[LOOP_COND:.*]] = icmp eq i64 %[[NEXT_INDEX]], %size
; CHECK-NEXT:    br i1 %[[LOOP_COND]], label %[[TAIL:.*]], label %[[THEN]]
;
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    %[[SIZE_SCALED:.*]] = shl nuw nsw i64 %size, 1
; CHECK-NEXT:    call void @llvm.memset.p0i8.i64(i8* %[[PTR_I8]], i8 15, i64 %[[SIZE_SCALED]], i32 2, i1 false)
; CHECK-NEXT:    br label %[[TAIL]]
;
; CHECK:       [[TAIL]]:
; CHECK-NEXT:    ret void
}

define void @set4(i32* %ptr, i64 %size) {
; CHECK-LABEL: define void @set4(
entry:
  %ptr.i8 = bitcast i32* %ptr to i8*
  %size.scaled = shl nuw nsw i64 %size, 2
  call void @llvm.memset.p0i8.i64(i8* %ptr.i8, i8 15, i64 %size.scaled, i32 4, i1 false)
  ret void
; CHECK:       entry:
; CHECK-NEXT:    %[[PTR_I8:.*]] = bitcast i32* %ptr to i8*
; CHECK-NEXT:    %[[ZERO_COND:.*]] = icmp ne i64 %size, 0
; CHECK-NEXT:    br i1 %[[ZERO_COND]], label %[[IF:.*]], label %[[TAIL:.*]]
;
; CHECK:       [[IF]]:
; CHECK-NEXT:    %[[COUNT_COND:.*]] = icmp ule i64 %size, 4
; CHECK-NEXT:    br i1 %[[COUNT_COND]], label %[[THEN:.*]], label %[[ELSE:.*]]
;
; CHECK:       [[THEN]]:
; CHECK-NEXT:    %[[INDEX:.*]] = phi i64 [ 0, %[[IF]] ], [ %[[NEXT_INDEX:.*]], %[[THEN]] ]
; CHECK-NEXT:    %[[INDEXED_DST:.*]] = getelementptr inbounds i32, i32* %ptr, i64 %[[INDEX]]
; CHECK-NEXT:    store i32 252645135, i32* %[[INDEXED_DST]], align 4
; CHECK-NEXT:    %[[NEXT_INDEX]] = add i64 %[[INDEX]], 1
; CHECK-NEXT:    %[[LOOP_COND:.*]] = icmp eq i64 %[[NEXT_INDEX]], %size
; CHECK-NEXT:    br i1 %[[LOOP_COND]], label %[[TAIL:.*]], label %[[THEN]]
;
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    %[[SIZE_SCALED:.*]] = shl nuw nsw i64 %size, 2
; CHECK-NEXT:    call void @llvm.memset.p0i8.i64(i8* %[[PTR_I8]], i8 15, i64 %[[SIZE_SCALED]], i32 4, i1 false)
; CHECK-NEXT:    br label %[[TAIL]]
;
; CHECK:       [[TAIL]]:
; CHECK-NEXT:    ret void
}

define void @set8(i64* %ptr, i64 %size) {
; CHECK-LABEL: define void @set8(
entry:
  %ptr.i8 = bitcast i64* %ptr to i8*
  %size.scaled = shl nuw nsw i64 %size, 3
  call void @llvm.memset.p0i8.i64(i8* %ptr.i8, i8 15, i64 %size.scaled, i32 8, i1 false)
  ret void
; CHECK:       entry:
; CHECK-NEXT:    %[[PTR_I8:.*]] = bitcast i64* %ptr to i8*
; CHECK-NEXT:    %[[ZERO_COND:.*]] = icmp ne i64 %size, 0
; CHECK-NEXT:    br i1 %[[ZERO_COND]], label %[[IF:.*]], label %[[TAIL:.*]]
;
; CHECK:       [[IF]]:
; CHECK-NEXT:    %[[COUNT_COND:.*]] = icmp ule i64 %size, 2
; CHECK-NEXT:    br i1 %[[COUNT_COND]], label %[[THEN:.*]], label %[[ELSE:.*]]
;
; CHECK:       [[THEN]]:
; CHECK-NEXT:    %[[INDEX:.*]] = phi i64 [ 0, %[[IF]] ], [ %[[NEXT_INDEX:.*]], %[[THEN]] ]
; CHECK-NEXT:    %[[INDEXED_DST:.*]] = getelementptr inbounds i64, i64* %ptr, i64 %[[INDEX]]
; CHECK-NEXT:    store i64 1085102592571150095, i64* %[[INDEXED_DST]], align 8
; CHECK-NEXT:    %[[NEXT_INDEX]] = add i64 %[[INDEX]], 1
; CHECK-NEXT:    %[[LOOP_COND:.*]] = icmp eq i64 %[[NEXT_INDEX]], %size
; CHECK-NEXT:    br i1 %[[LOOP_COND]], label %[[TAIL:.*]], label %[[THEN]]
;
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    %[[SIZE_SCALED:.*]] = shl nuw nsw i64 %size, 3
; CHECK-NEXT:    call void @llvm.memset.p0i8.i64(i8* %[[PTR_I8]], i8 15, i64 %[[SIZE_SCALED]], i32 8, i1 false)
; CHECK-NEXT:    br label %[[TAIL]]
;
; CHECK:       [[TAIL]]:
; CHECK-NEXT:    ret void
}

define void @copy1(i8* noalias %dst, i8* noalias %src, i64 %size) {
; CHECK-LABEL: define void @copy1(
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %size, i32 1, i1 false)
  ret void
; CHECK:       entry:
; CHECK-NEXT:    %[[ZERO_COND:.*]] = icmp ne i64 %size, 0
; CHECK-NEXT:    br i1 %[[ZERO_COND]], label %[[IF:.*]], label %[[TAIL:.*]]
;
; CHECK:       [[IF]]:
; CHECK-NEXT:    %[[COUNT_COND:.*]] = icmp ule i64 %size, 6
; CHECK-NEXT:    br i1 %[[COUNT_COND]], label %[[THEN:.*]], label %[[ELSE:.*]]
;
; CHECK:       [[THEN]]:
; CHECK-NEXT:    %[[INDEX:.*]] = phi i64 [ 0, %[[IF]] ], [ %[[NEXT_INDEX:.*]], %[[THEN]] ]
; CHECK-NEXT:    %[[INDEXED_DST:.*]] = getelementptr inbounds i8, i8* %dst, i64 %[[INDEX]]
; CHECK-NEXT:    %[[INDEXED_SRC:.*]] = getelementptr inbounds i8, i8* %src, i64 %[[INDEX]]
; CHECK-NEXT:    %[[LOAD:.*]] = load i8, i8* %[[INDEXED_SRC]], align 1
; CHECK-NEXT:    store i8 %[[LOAD]], i8* %[[INDEXED_DST]], align 1
; CHECK-NEXT:    %[[NEXT_INDEX]] = add i64 %[[INDEX]], 1
; CHECK-NEXT:    %[[LOOP_COND:.*]] = icmp eq i64 %[[NEXT_INDEX]], %size
; CHECK-NEXT:    br i1 %[[LOOP_COND]], label %[[TAIL:.*]], label %[[THEN]]
;
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %size, i32 1, i1 false)
; CHECK-NEXT:    br label %[[TAIL]]
;
; CHECK:       [[TAIL]]:
; CHECK-NEXT:    ret void
}

define void @copy2(i16* noalias %dst, i16* noalias %src, i64 %size) {
; CHECK-LABEL: define void @copy2(
entry:
  %dst.i8 = bitcast i16* %dst to i8*
  %src.i8 = bitcast i16* %src to i8*
  %size.scaled = shl nuw nsw i64 %size, 1
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst.i8, i8* %src.i8, i64 %size.scaled, i32 2, i1 false)
  ret void
; CHECK:       entry:
; CHECK-NEXT:    %[[DST_I8:.*]] = bitcast i16* %dst to i8*
; CHECK-NEXT:    %[[SRC_I8:.*]] = bitcast i16* %src to i8*
; CHECK-NEXT:    %[[ZERO_COND:.*]] = icmp ne i64 %size, 0
; CHECK-NEXT:    br i1 %[[ZERO_COND]], label %[[IF:.*]], label %[[TAIL:.*]]
;
; CHECK:       [[IF]]:
; CHECK-NEXT:    %[[COUNT_COND:.*]] = icmp ule i64 %size, 6
; CHECK-NEXT:    br i1 %[[COUNT_COND]], label %[[THEN:.*]], label %[[ELSE:.*]]
;
; CHECK:       [[THEN]]:
; CHECK-NEXT:    %[[INDEX:.*]] = phi i64 [ 0, %[[IF]] ], [ %[[NEXT_INDEX:.*]], %[[THEN]] ]
; CHECK-NEXT:    %[[INDEXED_DST:.*]] = getelementptr inbounds i16, i16* %dst, i64 %[[INDEX]]
; CHECK-NEXT:    %[[INDEXED_SRC:.*]] = getelementptr inbounds i16, i16* %src, i64 %[[INDEX]]
; CHECK-NEXT:    %[[LOAD:.*]] = load i16, i16* %[[INDEXED_SRC]], align 2
; CHECK-NEXT:    store i16 %[[LOAD]], i16* %[[INDEXED_DST]], align 2
; CHECK-NEXT:    %[[NEXT_INDEX]] = add i64 %[[INDEX]], 1
; CHECK-NEXT:    %[[LOOP_COND:.*]] = icmp eq i64 %[[NEXT_INDEX]], %size
; CHECK-NEXT:    br i1 %[[LOOP_COND]], label %[[TAIL:.*]], label %[[THEN]]
;
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    %[[SIZE_SCALED:.*]] = shl nuw nsw i64 %size, 1
; CHECK-NEXT:    call void @llvm.memcpy.p0i8.p0i8.i64(i8* %[[DST_I8]], i8* %[[SRC_I8]], i64 %[[SIZE_SCALED]], i32 2, i1 false)
; CHECK-NEXT:    br label %[[TAIL]]
;
; CHECK:       [[TAIL]]:
; CHECK-NEXT:    ret void
}

define void @copy4(i32* noalias %dst, i32* noalias %src, i64 %size) {
; CHECK-LABEL: define void @copy4(
entry:
  %dst.i8 = bitcast i32* %dst to i8*
  %src.i8 = bitcast i32* %src to i8*
  %size.scaled = shl nuw nsw i64 %size, 2
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst.i8, i8* %src.i8, i64 %size.scaled, i32 4, i1 false)
  ret void
; CHECK:       entry:
; CHECK-NEXT:    %[[DST_I8:.*]] = bitcast i32* %dst to i8*
; CHECK-NEXT:    %[[SRC_I8:.*]] = bitcast i32* %src to i8*
; CHECK-NEXT:    %[[ZERO_COND:.*]] = icmp ne i64 %size, 0
; CHECK-NEXT:    br i1 %[[ZERO_COND]], label %[[IF:.*]], label %[[TAIL:.*]]
;
; CHECK:       [[IF]]:
; CHECK-NEXT:    %[[COUNT_COND:.*]] = icmp ule i64 %size, 4
; CHECK-NEXT:    br i1 %[[COUNT_COND]], label %[[THEN:.*]], label %[[ELSE:.*]]
;
; CHECK:       [[THEN]]:
; CHECK-NEXT:    %[[INDEX:.*]] = phi i64 [ 0, %[[IF]] ], [ %[[NEXT_INDEX:.*]], %[[THEN]] ]
; CHECK-NEXT:    %[[INDEXED_DST:.*]] = getelementptr inbounds i32, i32* %dst, i64 %[[INDEX]]
; CHECK-NEXT:    %[[INDEXED_SRC:.*]] = getelementptr inbounds i32, i32* %src, i64 %[[INDEX]]
; CHECK-NEXT:    %[[LOAD:.*]] = load i32, i32* %[[INDEXED_SRC]], align 4
; CHECK-NEXT:    store i32 %[[LOAD]], i32* %[[INDEXED_DST]], align 4
; CHECK-NEXT:    %[[NEXT_INDEX]] = add i64 %[[INDEX]], 1
; CHECK-NEXT:    %[[LOOP_COND:.*]] = icmp eq i64 %[[NEXT_INDEX]], %size
; CHECK-NEXT:    br i1 %[[LOOP_COND]], label %[[TAIL:.*]], label %[[THEN]]
;
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    %[[SIZE_SCALED:.*]] = shl nuw nsw i64 %size, 2
; CHECK-NEXT:    call void @llvm.memcpy.p0i8.p0i8.i64(i8* %[[DST_I8]], i8* %[[SRC_I8]], i64 %[[SIZE_SCALED]], i32 4, i1 false)
; CHECK-NEXT:    br label %[[TAIL]]
;
; CHECK:       [[TAIL]]:
; CHECK-NEXT:    ret void
}

define void @copy8(i64* noalias %dst, i64* noalias %src, i64 %size) {
; CHECK-LABEL: define void @copy8(
entry:
  %dst.i8 = bitcast i64* %dst to i8*
  %src.i8 = bitcast i64* %src to i8*
  %size.scaled = shl nuw nsw i64 %size, 3
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst.i8, i8* %src.i8, i64 %size.scaled, i32 8, i1 false)
  ret void
; CHECK:       entry:
; CHECK-NEXT:    %[[DST_I8:.*]] = bitcast i64* %dst to i8*
; CHECK-NEXT:    %[[SRC_I8:.*]] = bitcast i64* %src to i8*
; CHECK-NEXT:    %[[ZERO_COND:.*]] = icmp ne i64 %size, 0
; CHECK-NEXT:    br i1 %[[ZERO_COND]], label %[[IF:.*]], label %[[TAIL:.*]]
;
; CHECK:       [[IF]]:
; CHECK-NEXT:    %[[COUNT_COND:.*]] = icmp ule i64 %size, 2
; CHECK-NEXT:    br i1 %[[COUNT_COND]], label %[[THEN:.*]], label %[[ELSE:.*]]
;
; CHECK:       [[THEN]]:
; CHECK-NEXT:    %[[INDEX:.*]] = phi i64 [ 0, %[[IF]] ], [ %[[NEXT_INDEX:.*]], %[[THEN]] ]
; CHECK-NEXT:    %[[INDEXED_DST:.*]] = getelementptr inbounds i64, i64* %dst, i64 %[[INDEX]]
; CHECK-NEXT:    %[[INDEXED_SRC:.*]] = getelementptr inbounds i64, i64* %src, i64 %[[INDEX]]
; CHECK-NEXT:    %[[LOAD:.*]] = load i64, i64* %[[INDEXED_SRC]], align 8
; CHECK-NEXT:    store i64 %[[LOAD]], i64* %[[INDEXED_DST]], align 8
; CHECK-NEXT:    %[[NEXT_INDEX]] = add i64 %[[INDEX]], 1
; CHECK-NEXT:    %[[LOOP_COND:.*]] = icmp eq i64 %[[NEXT_INDEX]], %size
; CHECK-NEXT:    br i1 %[[LOOP_COND]], label %[[TAIL:.*]], label %[[THEN]]
;
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    %[[SIZE_SCALED:.*]] = shl nuw nsw i64 %size, 3
; CHECK-NEXT:    call void @llvm.memcpy.p0i8.p0i8.i64(i8* %[[DST_I8]], i8* %[[SRC_I8]], i64 %[[SIZE_SCALED]], i32 8, i1 false)
; CHECK-NEXT:    br label %[[TAIL]]
;
; CHECK:       [[TAIL]]:
; CHECK-NEXT:    ret void
}

declare void @llvm.memcpy.p0i8.p0i8.i64(i8* writeonly, i8*, i64, i32, i1)

declare void @llvm.memset.p0i8.i64(i8* writeonly, i8, i64, i32, i1)
