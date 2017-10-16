; RUN: opt -S -PPCLowerMemIntrinsics -ppc-enable-memcpy-loops=true   \
; RUN: -mtriple=powerpc64le-unknown-linux-gnu -ppc-memcpy-loop-floor=0 \
; RUN: -mcpu=pwr8 %s| FileCheck -check-prefix=OPT %s
; RUN: opt -S -PPCLowerMemIntrinsics -ppc-enable-memcpy-loops=true \
; RUN: -mtriple=powerpc64-unknown-linux-gnu -mcpu=pwr7 %s | \
; RUN: FileCheck %s --check-prefix PWR7
; RUN: llc < %s  -ppc-enable-memcpy-loops=true   \
; RUN: -mtriple=powerpc64le-unknown-linux-gnu -mcpu=pwr8  -O0  | \
; RUN: FileCheck %s --check-prefix OPTNONE

declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i32, i1) #0

; Check that memcpy calls with a known zero length are removed.
define i8* @memcpy_zero_size(i8* %dst, i8* %src) {
entry:
  tail call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 0, i32 1, i1 false)
  ret i8* %dst

; OPT-LABEL: @memcpy_zero_size
; OPT-NEXT:  entry:
; OPT-NEXT:  ret i8* %dst
}

; Check that a memcpy call with a known size smaller then the loop operand
; type is handled properly.
define i8* @memcpy_small_size(i8* %dst, i8* %src) {
entry:
  tail call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 7, i32 1, i1 false)
  ret i8* %dst

; OPT-LABEL: @memcpy_small_size
; OPT-NEXT:  entry:
; OPT-NEXT:  [[SrcAsi32:%[0-9]+]] = bitcast i8* %src to i32*
; OPT-NEXT:  [[SrcGep:%[0-9]+]] = getelementptr inbounds i32, i32* [[SrcAsi32]], i64 0
; OPT-NEXT:  [[Load:%[0-9]+]] = load i32, i32* [[SrcGep]]
; OPT-NEXT:  [[DstAsi32:%[0-9]+]] = bitcast i8* %dst to i32*
; OPT-NEXT:  [[DstGep:%[0-9]+]] = getelementptr inbounds i32, i32* [[DstAsi32]], i64 0
; OPT-NEXT:  store i32 [[Load]], i32* [[DstGep]]
; OPT-NEXT:  [[SrcAsi16:%[0-9]+]] = bitcast i8* %src to i16*
; OPT-NEXT:  [[SrcGep2:%[0-9]+]] = getelementptr inbounds i16, i16* [[SrcAsi16]], i64 2
; OPT-NEXT:  [[Load2:%[0-9]+]] = load i16, i16* [[SrcGep2]]
; OPT-NEXT:  [[DstAsi16:%[0-9]+]] = bitcast i8* %dst to i16*
; OPT-NEXT:  [[DstGep2:%[0-9]+]] = getelementptr inbounds i16, i16* [[DstAsi16]], i64 2
; OPT-NEXT:  store i16 [[Load2]], i16* [[DstGep2]]
; OPT-NEXT:  [[SrcGep3:%[0-9]+]] = getelementptr inbounds i8, i8* %src, i64 6
; OPT-NEXT:  [[Load3:%[0-9]+]] = load i8, i8* [[SrcGep3]]
; OPT-NEXT:  [[DstGep3:%[0-9]+]] = getelementptr inbounds i8, i8* %dst, i64 6
; OPT-NEXT:  store i8 [[Load3]], i8* [[DstGep3]]
; OPT-NEXT:  ret i8* %dst
}

; Check the expansion of a memcpy call with compile-time size.
define i8* @memcpy_known_size(i8* %dst, i8* %src) {
entry:
  tail call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 100, i32 1, i1 false)
  ret i8* %dst
; OPT-LABEL: @memcpy_known_size
; OPT:       entry:
; OPT-NEXT:  [[SrcCast:%[0-9]+]] = bitcast i8* %src to i64*
; OPT-NEXT:  [[DstCast:%[0-9]+]] = bitcast i8* %dst to i64*
; OPT-NEXT:  br label %load-store-loop

; OPT:       load-store-loop:
; OPT-NEXT:  %loop-index = phi i64 [ 0, %entry ], [ [[IndexInc:%[0-9]+]], %load-store-loop ]
; OPT-NEXT:  [[SrcGep:%[0-9]+]] = getelementptr inbounds i64, i64* [[SrcCast]], i64 %loop-index
; OPT-NEXT:  [[Load:%[0-9]+]] = load i64, i64* [[SrcGep]]
; OPT-NEXT:  [[DstGep:%[0-9]+]] = getelementptr inbounds i64, i64* [[DstCast]], i64 %loop-index
; OPT-NEXT:  store i64 [[Load]], i64* [[DstGep]]
; OPT-NEXT:  [[IndexInc]] = add i64 %loop-index, 1
; OPT-NEXT:  [[CMP:%[0-9]+]] = icmp ult i64 [[IndexInc]], 12
; OPT-NEXT:  br i1 [[CMP]], label %load-store-loop, label %memcpy-split

; OPT:       memcpy-split:
; OPT-NEXT:  [[SrcAsi32:%[0-9]+]] = bitcast i64* [[SrcCast]] to i32*
; OPT-NEXT:  [[SrcGep2:%[0-9]+]] = getelementptr inbounds i32, i32* [[SrcAsi32]], i64 24
; OPT-NEXT:  [[Load2:%[0-9]+]] = load i32, i32* [[SrcGep2]]
; OPT-NEXT:  [[DstAsi32:%[0-9]+]] = bitcast i64* [[DstCast]] to i32*
; OPT-NEXT:  [[DstGep2:%[0-9]+]] = getelementptr inbounds i32, i32* [[DstAsi32]], i64 24
; OPT-NEXT:  store i32 [[Load2]], i32* [[DstGep2]]
; OPT-NEXT:  ret i8* %dst
}


; Check the expansion of a memcpy whose size argument is not a compile time
; constant.
define i8* @memcpy_unkown_size(i8* %dst, i8* %src, i64 %len) {
entry:
  tail call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %len, i32 1, i1 false)
  ret i8* %dst

; OPT-LABEL: @memcpy_unkown_size
; OPT:       entry:
; OPT-NEXT:  [[SrcCast:%[0-9]+]] = bitcast i8* %src to i64*
; OPT-NEXT:  [[DstCast:%[0-9]+]] = bitcast i8* %dst to i64*
; OPT-NEXT:  [[LoopCount:%[0-9]+]] = udiv i64 %len, 8
; OPT-NEXT:  [[ResBytes:%[0-9]+]] = urem i64 %len, 8
; OPT-NEXT:  [[BytesCopied:%[0-9]+]] = sub i64 %len, [[ResBytes]]
; OPT-NEXT:  [[Cmp:%[0-9]+]] = icmp ne i64 [[LoopCount]], 0
; OPT-NEXT:  br i1 [[Cmp]], label %loop-memcpy-expansion, label %loop-memcpy-residual-header

; OPT:       post-loop-memcpy-expansion:
; OPT-NEXT:  ret i8* %dst

; OPT:       loop-memcpy-expansion:
; OPT-NEXT:  %loop-index = phi i64 [ 0, %entry ], [ [[IndexInc:%[0-9]+]], %loop-memcpy-expansion ]
; OPT-NEXT:  [[SrcGep:%[0-9]+]] = getelementptr inbounds i64, i64* [[SrcCast]], i64 %loop-index
; OPT-NEXT:  [[Load:%[0-9]+]] = load i64, i64* [[SrcGep]]
; OPT-NEXT:  [[DstGep:%[0-9]+]] = getelementptr inbounds i64, i64* [[DstCast]], i64 %loop-index
; OPT-NEXT:  store i64 [[Load]], i64* [[DstGep]]
; OPT-NEXT:  [[IndexInc]] = add i64 %loop-index, 1
; OPT-NEXT:  [[LoopCmp:%[0-9]+]] = icmp ult i64 [[IndexInc]], [[LoopCount]]
; OPT-NEXT:  br i1 [[LoopCmp]], label %loop-memcpy-expansion, label %loop-memcpy-residual-header

; OPT:       loop-memcpy-residual:
; OPT-NEXT:  %residual-loop-index = phi i64 [ 0, %loop-memcpy-residual-header ], [ [[ResIndexInc:%[0-9]+]], %loop-memcpy-residual ]
; OPT-NEXT:  [[SrcAsi8:%[0-9]+]] = bitcast i64* [[SrcCast]] to i8*
; OPT-NEXT:  [[DstAsi8:%[0-9]+]] = bitcast i64* [[DstCast]] to i8*
; OPT-NEXT:  [[ResIndex:%[0-9]+]] = add i64 [[BytesCopied]], %residual-loop-index
; OPT-NEXT:  [[SrcGep2:%[0-9]+]] = getelementptr inbounds i8, i8* [[SrcAsi8]], i64 [[ResIndex]]
; OPT-NEXT:  [[Load2:%[0-9]+]] = load i8, i8* [[SrcGep2]]
; OPT-NEXT:  [[DstGep2:%[0-9]+]] = getelementptr inbounds i8, i8* [[DstAsi8]], i64 [[ResIndex]]
; OPT-NEXT:  store i8 [[Load2]], i8* [[DstGep2]]
; OPT-NEXT:  [[ResIndexInc]] = add i64 %residual-loop-index, 1
; OPT-NEXT:  [[RCmp:%[0-9]+]] = icmp ult i64 [[ResIndexInc]], [[ResBytes]]
; OPT-NEXT:  br i1 [[RCmp]], label %loop-memcpy-residual, label %post-loop-memcpy-expansion

; OPT:       loop-memcpy-residual-header:
; OPT-NEXT:  [[RHCmp:%[0-9]+]] = icmp ne i64 [[ResBytes]], 0
; OPT-NEXT:  br i1 [[RHCmp]], label %loop-memcpy-residual, label %post-loop-memcpy-expansion
}

; Ensure the pass doens't expand memcpy calls when compiling a function with an
; unspported target_cpu attribute.
define i8* @memcpy_power7(i8* %dst, i8* %src, i64 %len) #1 {
entry:
  tail call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %len, i32 1, i1 false)
  ret i8* %dst
; PWR7-LABEL: @memcpy_power7
; PWR7:       tail call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %len, i32 1, i1 false)
}

; Ensure the pass doens't expand calls in a function compiled for size.
define i8* @memcpy_opt_small(i8* %dst, i8* %src, i64 %len) #2 {
  entry:
  tail call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %len, i32 1, i1 false)
  ret i8* %dst
; OPT-LABEL: @memcpy_opt_small
; OPT:       tail call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %len, i32 1, i1 false)
}

; Ensure the pass doesn't expand calls on functions not compiled with
; optimizations.
define i8* @memcpy_opt_none(i8* %dst, i8* %src, i64 %len) {
  entry:
  tail call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %len, i32 1, i1 false)
  ret i8* %dst
; OPTNONE-LABEL: @memcpy_opt_none
; OPTNONE:       bl memcpy
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { "target-cpu"="power7" }
attributes #2 = { "target-cpu"="power8" optsize }
