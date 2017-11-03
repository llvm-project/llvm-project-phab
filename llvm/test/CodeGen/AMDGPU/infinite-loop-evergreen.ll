; XFAIL: *
; REQUIRES: asserts
; RUN: llc -march=r600 -mtriple=r600---amdgiz -mcpu=cypress < %s | FileCheck %s

define amdgpu_kernel void @inf_loop_irreducible_cfg() nounwind {
entry:
  br label %block

block:
  br label %block
}
