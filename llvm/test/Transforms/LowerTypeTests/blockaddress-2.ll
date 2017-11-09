; RUN: opt -S %s -lowertypetests | FileCheck %s

; CHECK: @badfileops = internal global %struct.f { i32 (i32*)* @bad_f, i32 (i32*)* @bad_f }
; CHECK: @bad_f = internal alias i32 (i32*), bitcast (void ()* @.cfi.jumptable to i32 (i32*)*)
; CHECK: define internal i32 @bad_f.cfi(i32* nocapture readnone) !type !0 {
; CHECK-NEXT:  ret i32 9

target triple = "x86_64-unknown-linux"

%struct.f = type { i32 (i32*)*, i32 (i32*)* }
@llvm.used = external global [43 x i8*], section "llvm.metadata"
@badfileops = internal global %struct.f { i32 (i32*)* @bad_f, i32 (i32*)* @bad_f }, align 8

declare i1 @llvm.type.test(i8*, metadata)

define internal i32 @bad_f(i32* nocapture readnone) !type !1 {
  ret i32 9
}

define internal fastcc i32 @do_f(i32, i32, i32*, i32, i64, i32) unnamed_addr !type !2 {
  %7 = tail call i1 @llvm.type.test(i8* undef, metadata !"_ZTSFiP4fileP3uioP5ucrediP6threadE"), !nosanitize !3
  ret i32 undef
}

!1 = !{i64 0, !"_ZTSFiP4fileP3uioP5ucrediP6threadE"}
!2 = !{i64 0, !"_ZTSFiP6threadiP4fileP3uioliE"}
!3 = !{}
