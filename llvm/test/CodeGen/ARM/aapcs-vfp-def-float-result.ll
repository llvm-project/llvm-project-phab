; RUN: not llc -mtriple=armv4--linux-gnueabi -mattr=-vfp2 %s -o - 2>&1 | FileCheck %s
; RUN: not llc -mtriple=thumbv4--linux-gnueabi -mattr=-vfp2,-neon %s -o - 2>&1 | FileCheck %s
; RUN: not llc -mtriple=thumbv4--linux-gnueabi -mattr=+vfp2,-neon %s -o - 2>&1 | FileCheck %s
; RUN: llc -mtriple=armv4--linux-gnueabi -mattr=+vfp2 %s -o - 2>&1 | not FileCheck %s
; RUN: llc -mtriple=thumbv7--linux-gnueabi -mattr=+vfp2 %s -o - 2>&1 | not FileCheck %s

define arm_aapcs_vfpcc float @f() {
; CHECK: LLVM ERROR: The aapcs_vfp calling convention is not supported on this target.
  ret float 1.0
}
