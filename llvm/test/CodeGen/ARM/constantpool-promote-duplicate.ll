; RUN: llc -relocation-model=static -arm-promote-constant < %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-v128:64:128-a:0:32-n32-S64"
target triple = "thumbv7m--linux-gnu"

@const1 = private unnamed_addr constant i32 0, align 4
@const2 = private unnamed_addr constant i32 0, align 4

; const1 and const2 both need labels for debug info, but will be coalesced into
; a single constpool entry

; CHECK-LABEL: @test1
; CHECK-DAG: const1:
; CHECK-DAG: const2:
; CHECK: .fnend
define void @test1() #0 {
  %1 = load i32, i32* @const1, align 4
  call void @a(i32 %1) #1
  %2 = load i32, i32* @const2, align 4
  call void @a(i32 %2) #1
  ret void
}

declare void @a(i32) #1

attributes #0 = { nounwind  "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind }
