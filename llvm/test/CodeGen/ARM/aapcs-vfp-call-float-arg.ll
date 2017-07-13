; RUN: not llc -mtriple=armv4--linux-gnueabi -mattr=-vfp2 %s -o - 2>&1 | FileCheck %s
; RUN: not llc -mtriple=thumbv4--linux-gnueabi -mattr=-vfp2,-neon %s -o - 2>&1 | FileCheck %s
; RUN: not llc -mtriple=thumbv4--linux-gnueabi -mattr=+vfp2,-neon %s -o - 2>&1 | FileCheck %s
; RUN: llc -mtriple=armv4--linux-gnueabi -mattr=+vfp2 %s -o - 2>&1 | not FileCheck %s
; RUN: llc -mtriple=thumbv7--linux-gnueabi -mattr=+vfp2 %s -o - 2>&1 | not FileCheck %s

define arm_aapcscc void @g() {
; CHECK: LLVM ERROR: The aapcs_vfp calling convention is not supported on this target.
  call arm_aapcs_vfpcc void @f(float 1.0)
  ret void
}

declare arm_aapcs_vfpcc void @f(float)
