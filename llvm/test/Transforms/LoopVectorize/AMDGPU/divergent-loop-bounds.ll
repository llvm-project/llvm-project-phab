; RUN: opt -S -mtriple=amdgcn-unknown-amdhsa -mcpu=gfx900 -loop-vectorize -simplifycfg < %s | FileCheck -check-prefixes=GCN,GFX9 %s
; RUN: opt -S -mtriple=amdgcn-unknown-amdhsa -mcpu=fiji -loop-vectorize -simplifycfg < %s | FileCheck -check-prefixes=GCN,VI %s

; It may make sense to vectorize this if the condition is uniform, but
; assume that it isn't for now.

; GCN-LABEL: @small_loop_i16_unknown_uniform_size(
; GCN: load i16
; GCN: add nsw i16
; GCN: store i16
; GCN: br i1 %cond
define amdgpu_kernel void @small_loop_i16_unknown_uniform_size(i16 addrspace(1)* nocapture %inArray, i16 %size) #0 {
entry:
  %cmp = icmp sgt i16 %size, 0
  br i1 %cmp, label %loop, label %exit

loop:                                          ; preds = %entry, %loop
  %iv = phi i16 [ %iv1, %loop ], [ 0, %entry ]
  %gep = getelementptr inbounds i16, i16 addrspace(1)* %inArray, i16 %iv
  %load = load i16, i16 addrspace(1)* %gep, align 4
  %add = add nsw i16 %load, 6
  store i16 %add, i16 addrspace(1)* %gep, align 4
  %iv1 = add i16 %iv, 1
  %cond = icmp eq i16 %iv1, %size
  br i1 %cond, label %exit, label %loop

exit:                                         ; preds = %loop, %entry
  ret void
}

; GCN-LABEL: @small_loop_i16_unknown_divergent_size(
; GCN: load i16
; GCN: add nsw i16
; GCN: store i16
; GCN: br i1 %cond
define amdgpu_kernel void @small_loop_i16_unknown_divergent_size(i16 addrspace(1)* nocapture %inArray, i16 addrspace(1)* %size.ptr) #0 {
entry:
  %tid = call i32 @llvm.amdgcn.workitem.id.x()
  %size.gep = getelementptr inbounds i16, i16 addrspace(1)* %size.ptr, i32 %tid
  %size = load i16, i16 addrspace(1)* %size.gep
  %cmp = icmp sgt i16 %size, 0
  br i1 %cmp, label %loop, label %exit

loop:                                          ; preds = %entry, %loop
  %iv = phi i16 [ %iv1, %loop ], [ 0, %entry ]
  %gep = getelementptr inbounds i16, i16 addrspace(1)* %inArray, i16 %iv
  %load = load i16, i16 addrspace(1)* %gep, align 4
  %add = add nsw i16 %load, 6
  store i16 %add, i16 addrspace(1)* %gep, align 4
  %iv1 = add i16 %iv, 1
  %cond = icmp eq i16 %iv1, %size
  br i1 %cond, label %exit, label %loop

exit:                                         ; preds = %loop, %entry
  ret void
}

; GCN-LABEL: @small_loop_i16_256(
; GFX9: load <2 x i16>
; GFX9: add nsw <2 x i16>
; GFX9: store <2 x i16>
; GFX9: add i32 %index, 2
; GFX9: br i1

; VI-NOT: <2 x i16>
define amdgpu_kernel void @small_loop_i16_256(i16 addrspace(1)* nocapture %inArray) #0 {
entry:
  br label %loop

loop:                                          ; preds = %entry, %loop
  %iv = phi i16 [ %iv1, %loop ], [ 0, %entry ]
  %gep = getelementptr inbounds i16, i16 addrspace(1)* %inArray, i16 %iv
  %load = load i16, i16 addrspace(1)* %gep, align 4
  %add = add nsw i16 %load, 6
  store i16 %add, i16 addrspace(1)* %gep, align 4
  %iv1 = add i16 %iv, 1
  %cond = icmp eq i16 %iv1, 127
  br i1 %cond, label %exit, label %loop

exit:                                         ; preds = %loop, %entry
  ret void
}

; Not divisible by vectorize factor of 2
; GCN-LABEL: @small_loop_i16_255(
; GFX9: load <2 x i16>
; GFX9: add nsw <2 x i16>
; GFX9: store <2 x i16>
; GFX9: add i32 %index, 2
; GFX9: br i1

; VI-NOT: <2 x i16>
define amdgpu_kernel void @small_loop_i16_255(i16 addrspace(1)* nocapture %inArray) #0 {
entry:
  br label %loop

loop:                                          ; preds = %entry, %loop
  %iv = phi i16 [ %iv1, %loop ], [ 0, %entry ]
  %gep = getelementptr inbounds i16, i16 addrspace(1)* %inArray, i16 %iv
  %load = load i16, i16 addrspace(1)* %gep, align 4
  %add = add nsw i16 %load, 6
  store i16 %add, i16 addrspace(1)* %gep, align 4
  %iv1 = add i16 %iv, 1
  %cond = icmp eq i16 %iv1, 127
  br i1 %cond, label %exit, label %loop

exit:                                         ; preds = %loop, %entry
  ret void
}

declare i32 @llvm.amdgcn.workitem.id.x() #1

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }
