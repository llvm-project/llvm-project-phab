; Do setup work for all below tests: generate bitcode and combined index
; RUN: opt -module-summary %s -o %t.bc
; RUN: opt -module-summary %p/Inputs/import_weak_type.ll -o %t2.bc
; RUN: llvm-lto -thinlto-action=thinlink -o %t3.bc %t.bc %t2.bc

; Check that we import correctly the imported weak type to replace declaration here
; RUN: llvm-lto -thinlto-action=import %t.bc -thinlto-index=%t3.bc -force-import-weak -o - | llvm-dis -o - | FileCheck %s
; CHECK: define weak void @foo()


target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"
target triple = "amdgcn--amdhsa-hcc"

declare extern_weak void @foo()
define weak_odr amdgpu_kernel void @main() {
    call void @foo()
  ret void
}

