; RUN: llc -mtriple=arm -mattr=+neon,+fullfp16 %s -o - | FileCheck %s

define void @vst1lanehalf(half* %A, <4 x half>* %B) nounwind {
;CHECK-LABEL: vst1lanehalf:
;Check the alignment value.  Max for this instruction is 16 bits:
;CHECK: vst1.16 {d16[2]}, [r0:16]
	%tmp1 = load <4 x half>, <4 x half>* %B
        %tmp2 = extractelement <4 x half> %tmp1, i32 2
        store half %tmp2, half* %A, align 8
	ret void
}

define void @vst2lanehalf(half* %A, <4 x half>* %B) nounwind {
;CHECK-LABEL: vst2lanehalf:
;Check the alignment value.  Max for this instruction is 32 bits:
;CHECK: vst2.16 {d16[1], d17[1]}, [r0:32]
	%tmp0 = bitcast half* %A to i8*
	%tmp1 = load <4 x half>, <4 x half>* %B
	call void @llvm.arm.neon.vst2lane.p0i8.v4f16(i8* %tmp0, <4 x half> %tmp1, <4 x half> %tmp1, i32 1, i32 8)
	ret void
}

;Check for a post-increment updating store with register increment.
define void @vst2lanehalf_update(half** %ptr, <4 x half>* %B, i32 %inc) nounwind {
;CHECK-LABEL: vst2lanehalf_update:
;CHECK: vst2.16 {d16[1], d17[1]}, [r1], r2
	%A = load half*, half** %ptr
	%tmp0 = bitcast half* %A to i8*
	%tmp1 = load <4 x half>, <4 x half>* %B
	call void @llvm.arm.neon.vst2lane.p0i8.v4f16(i8* %tmp0, <4 x half> %tmp1, <4 x half> %tmp1, i32 1, i32 2)
	%tmp2 = getelementptr half, half* %A, i32 %inc
	store half* %tmp2, half** %ptr
	ret void
}

declare void @llvm.arm.neon.vst2lane.p0i8.v4f16(i8*, <4 x half>, <4 x half>, i32, i32) nounwind

define void @vst3lanehalf(half* %A, <4 x half>* %B) nounwind {
;CHECK-LABEL: vst3lanehalf:
;Check the (default) alignment value.  VST3 does not support alignment.
;CHECK: vst3.16 {d16[1], d17[1], d18[1]}, [r0]
	%tmp0 = bitcast half* %A to i8*
	%tmp1 = load <4 x half>, <4 x half>* %B
	call void @llvm.arm.neon.vst3lane.p0i8.v4f16(i8* %tmp0, <4 x half> %tmp1, <4 x half> %tmp1, <4 x half> %tmp1, i32 1, i32 8)
	ret void
}

declare void @llvm.arm.neon.vst3lane.p0i8.v4f16(i8*, <4 x half>, <4 x half>, <4 x half>, i32, i32) nounwind

define void @vst4lanehalf(half* %A, <4 x half>* %B) nounwind {
;CHECK-LABEL: vst4lanehalf:
;CHECK: vst4.16
	%tmp0 = bitcast half* %A to i8*
	%tmp1 = load <4 x half>, <4 x half>* %B
	call void @llvm.arm.neon.vst4lane.p0i8.v4f16(i8* %tmp0, <4 x half> %tmp1, <4 x half> %tmp1, <4 x half> %tmp1, <4 x half> %tmp1, i32 1, i32 1)
	ret void
}

declare void @llvm.arm.neon.vst4lane.p0i8.v4f16(i8*, <4 x half>, <4 x half>, <4 x half>, <4 x half>, i32, i32) nounwind
