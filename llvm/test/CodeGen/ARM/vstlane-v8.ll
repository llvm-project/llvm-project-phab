; RUN: llc -mtriple=arm -mattr=+neon,+fullfp16 %s -o - | FileCheck %s

define void @vst1laneQhalf(half* %A, <8 x half>* %B) nounwind {
;CHECK-LABEL: vst1laneQhalf:
;CHECK: vst1.16 {d17[1]}, [r0:16]
	%tmp1 = load <8 x half>, <8 x half>* %B
  %tmp2 = extractelement <8 x half> %tmp1, i32 5
  store half %tmp2, half* %A, align 8
	ret void
}

define void @vst2laneQhalf(half* %A, <8 x half>* %B) nounwind {
;CHECK-LABEL: vst2laneQhalf:
;Check the (default) alignment.
;CHECK: vst2.16 {d17[1], d19[1]}, [r0]
	%tmp0 = bitcast half* %A to i8*
	%tmp1 = load <8 x half>, <8 x half>* %B
	call void @llvm.arm.neon.vst2lane.p0i8.v8f16(i8* %tmp0, <8 x half> %tmp1, <8 x half> %tmp1, i32 5, i32 1)
	ret void
}

declare void @llvm.arm.neon.vst2lane.p0i8.v8f16(i8*, <8 x half>, <8 x half>, i32, i32) nounwind

define void @vst3laneQhalf(half* %A, <8 x half>* %B) nounwind {
;CHECK-LABEL: vst3laneQhalf:
;Check the (default) alignment value.  VST3 does not support alignment.
;CHECK: vst3.16 {d17[2], d19[2], d21[2]}, [r0]
	%tmp0 = bitcast half* %A to i8*
	%tmp1 = load <8 x half>, <8 x half>* %B
	call void @llvm.arm.neon.vst3lane.p0i8.v8f16(i8* %tmp0, <8 x half> %tmp1, <8 x half> %tmp1, <8 x half> %tmp1, i32 6, i32 8)
	ret void
}

declare void @llvm.arm.neon.vst3lane.p0i8.v8f16(i8*, <8 x half>, <8 x half>, <8 x half>, i32, i32) nounwind

define void @vst4laneQhalf(half* %A, <8 x half>* %B) nounwind {
;CHECK-LABEL: vst4laneQhalf:
;Check the alignment value.  Max for this instruction is 64 bits:
;CHECK: vst4.16 {d17[3], d19[3], d21[3], d23[3]}, [r0:64]
	%tmp0 = bitcast half* %A to i8*
	%tmp1 = load <8 x half>, <8 x half>* %B
	call void @llvm.arm.neon.vst4lane.p0i8.v8f16(i8* %tmp0, <8 x half> %tmp1, <8 x half> %tmp1, <8 x half> %tmp1, <8 x half> %tmp1, i32 7, i32 16)
	ret void
}

define <8 x half> @variable_insertelement(<8 x half> %a, half %b, i32 %c) nounwind readnone {
;CHECK-LABEL: variable_insertelement:
    %r = insertelement <8 x half> %a, half %b, i32 %c
    ret <8 x half> %r
}

declare void @llvm.arm.neon.vst4lane.p0i8.v8f16(i8*, <8 x half>, <8 x half>, <8 x half>, <8 x half>, i32, i32) nounwind
