; Disable shrink-wrapping on the first test otherwise we wouldn't
; exerce the path for PR18136.
; RUN: llc -mtriple=thumbv7-apple-none-macho < %s -enable-shrink-wrap=false | FileCheck %s


declare void @bar(i8*)

%bigVec = type [2 x double]

@var = global %bigVec zeroinitializer

define arm_aapcs_vfpcc double @check_vfp_no_return_clobber() minsize {
; CHECK-LABEL: check_vfp_no_return_clobber:
; CHECK: push {r[[GLOBREG:[0-9]+]], lr}
; CHECK: vpush {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9}
; CHECK-NOT: sub sp,
; ...
; CHECK: add sp, #64
; CHECK: vpop {d8, d9}
; CHECK: pop {r[[GLOBREG]], pc}

  %var = alloca i8, i32 64

  %tmp = load %bigVec, %bigVec* @var
  call void @bar(i8* %var)
  store %bigVec %tmp, %bigVec* @var

  ret double 1.0
}
