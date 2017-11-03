; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -verify-machineinstrs -mcpu=pwr8 -ppc-enable-memcpy-loops=true -ppc-memcpy-unknown-loops=true < %s | FileCheck %s

target datalayout = "e-m:e-i64:64-n32:64"
target triple = "powerpc64le-unknown-linux-gnu"

; Function Attrs: nounwind
define void @testMemcpyVersion(i8* nocapture %src, i8* nocapture readonly %dest, i32 signext %size) local_unnamed_addr #0 {
; CHECK-LABEL: testMemcpyVersion:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    mflr 0
; CHECK-NEXT:    std 0, 16(1)
; CHECK-NEXT:    stdu 1, -32(1)
; CHECK-NEXT:    .cfi_def_cfa_offset 32
; CHECK-NEXT:    .cfi_offset lr, 16
; CHECK-NEXT:    bl memcpy
; CHECK-NEXT:    nop
; CHECK-NEXT:    addi 1, 1, 32
; CHECK-NEXT:    ld 0, 16(1)
; CHECK-NEXT:    mtlr 0
; CHECK-NEXT:    blr
entry:
  %conv = sext i32 %size to i64
  tail call void @llvm.memcpy.p0i8.p0i8.i64(i8* %src, i8* %dest, i64 %conv, i32 1, i1 false)
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i32, i1) #1

