; RUN: opt -O1 -S -mtriple=amdgcn-unknown-amdhsa -amdgpu-internalize-symbols < %s | FileCheck -check-prefix=ALL -check-prefix=OPT %s
; RUN: opt -O0 -S -mtriple=amdgcn-unknown-amdhsa -amdgpu-internalize-symbols < %s | FileCheck -check-prefix=ALL -check-prefix=OPTNONE %s

; OPT-NOT: gvar_unused
; OPTNONE: gvar_unused

@gvar_unused = addrspace(1) global i32 undef, align 4

; CHECK: gvar_used
@gvar_used = addrspace(1) global i32 undef, align 4

; CHECK: define internal void @func_used(
define void @func_used(i32 addrspace(1)* %out, i32 %tid) #2 {
entry:
  store i32 %tid, i32 addrspace(1)* %out
  ret void
}

; CHECK: define amdgpu_kernel void @foo_unused(
define amdgpu_kernel void @foo_unused(i32 addrspace(1)* %out) local_unnamed_addr #1 {
entry:
  store i32 1, i32 addrspace(1)* %out
  ret void
}

; CHECK: define amdgpu_kernel void @foo_used(
define amdgpu_kernel void @foo_used(i32 addrspace(1)* %out, i32 %tid) local_unnamed_addr #1 {
entry:
  store i32 %tid, i32 addrspace(1)* %out
  ret void
}

; CHECK: define amdgpu_kernel void @main_kernel()
define amdgpu_kernel void @main_kernel() {
entry:
  %tid = call i32 @llvm.amdgcn.workitem.id.x()
  tail call void @foo_used(i32 addrspace(1)* @gvar_used, i32 %tid)
  tail call void @func_used(i32 addrspace(1)* @gvar_used, i32 %tid)
  ret void
}

declare i32 @llvm.amdgcn.workitem.id.x() #0

attributes #0 = { nounwind readnone }
attributes #1 = { alwaysinline nounwind }
attributes #2 = { noinline nounwind }
