; RUN: opt < %s -inline -S | FileCheck %s --check-prefix=CHECK --check-prefix=INLINE
; RUN: opt < %s -inline -inline-threshold=1 -disable-cygprofile-inlining -S | FileCheck %s --check-prefix=CHECK --check-prefix=NOINLINE

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare void @__cyg_profile_func_enter(i8*, i8*)
declare void @__cyg_profile_func_exit(i8*, i8*)
declare i8* @llvm.returnaddress(i32)

define internal void @g() {
entry:
  %callsite = call i8* @llvm.returnaddress(i32 0)
  call void @__cyg_profile_func_enter(i8* bitcast (void ()* @g to i8*), i8* %callsite)
  %callsite1 = call i8* @llvm.returnaddress(i32 0)
  call void @__cyg_profile_func_exit(i8* bitcast (void ()* @g to i8*), i8* %callsite1)
  ret void
}

define void @f() {
; CHECK-LABEL: @f
; The call to @g should be inlined, but whether or not @g's cygprofile calls
; should be inlined depends on the use of -disable-cygprofile-inlining.

; CHECK: call void @__cyg_profile_func_enter
; CHECK-NOT: call void @g()
; INLINE: call void @__cyg_profile_func_enter
; NOINLINE-NOT: call void @__cyg_profile_func_enter
; CHECK: call void @__cyg_profile_func_exit
; INLINE: call void @__cyg_profile_func_exit
; NOINLINE-NOT: call void @__cyg_profile_func_exit

entry:
  %callsite = call i8* @llvm.returnaddress(i32 0)
  call void @__cyg_profile_func_enter(i8* bitcast (void ()* @f to i8*), i8* %callsite)
  call void @g()
  %callsite1 = call i8* @llvm.returnaddress(i32 0)
  call void @__cyg_profile_func_exit(i8* bitcast (void ()* @f to i8*), i8* %callsite1)
  ret void
}
