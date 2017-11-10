; RUN: opt -passes="function(ee-instrument),cgscc(inline),function(post-inline-ee-instrument)" -S < %s | FileCheck %s

target datalayout = "E-m:e-i64:64-n32:64"
target triple = "powerpc64-bgq-linux"

define void @leaf_function() #0 {
entry:
  ret void

; CHECK-LABEL: define void @leaf_function()
; CHECK: entry:
; CHECK-NEXT: call void @mcount()
; CHECK-NEXT: %0 = call i8* @llvm.returnaddress(i32 0)
; CHECK-NEXT: call void @__cyg_profile_func_enter(i8* bitcast (void ()* @leaf_function to i8*), i8* %0)
; CHECK-NEXT: %1 = call i8* @llvm.returnaddress(i32 0)
; CHECK-NEXT: call void @__cyg_profile_func_exit(i8* bitcast (void ()* @leaf_function to i8*), i8* %1)
; CHECK-NEXT: ret void
}


define void @root_function() #0 {
entry:
  call void @leaf_function()
  ret void

; CHECK-LABEL: define void @root_function()
; CHECK: entry:
; CHECK-NEXT: call void @mcount()

; CHECK-NEXT %0 = call i8* @llvm.returnaddress(i32 0)
; CHECK-NEXT call void @__cyg_profile_func_enter(i8* bitcast (void ()* @root_function to i8*), i8* %0)

; Entry and exit calls, inlined from @leaf_function()
; CHECK-NEXT %1 = call i8* @llvm.returnaddress(i32 0)
; CHECK-NEXT call void @__cyg_profile_func_enter(i8* bitcast (void ()* @leaf_function to i8*), i8* %1)
; CHECK-NEXT %2 = call i8* @llvm.returnaddress(i32 0)
; CHECK-NEXT call void @__cyg_profile_func_exit(i8* bitcast (void ()* @leaf_function to i8*), i8* %2)
; CHECK-NEXT %3 = call i8* @llvm.returnaddress(i32 0)

; CHECK-NEXT call void @__cyg_profile_func_exit(i8* bitcast (void ()* @root_function to i8*), i8* %3)
; CHECK-NEXT ret void
}

attributes #0 = { "instrument-function-entry-inlined"="mcount" "instrument-function-entry"="__cyg_profile_func_enter" "instrument-function-exit"="__cyg_profile_func_exit" }
