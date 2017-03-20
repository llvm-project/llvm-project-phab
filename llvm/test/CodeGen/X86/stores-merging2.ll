; RUN: llc %s -o - | FileCheck %s

target datalayout = "e-m:e-p:32:32-f64:32:64-f80:32-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"



@ptr = global [32 x i32] zeroinitializer, align 4
@strptr = global [4 x i8] zeroinitializer, align 1

; FindBetterChains in SelectionDAG should give up and the 4 1-byte
; stores will remain in a chain. We should still be able to merge them
; into a 32-bit store.

; CHECK-LABEL: @foo
; CHECK: movl	$1684234849, strpt

define i32 @foo() {
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 0)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 1)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 2)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 3)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 4)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 5)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 6)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 7)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 8)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 9)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 10)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 11)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 12)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 13)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 14)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 15)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 16)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 17)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 18)
  store i32 0, i32* getelementptr inbounds ([32 x i32], [32 x i32]* @ptr, i32 0, i32 19)
  store i8 97, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @strptr, i32 0, i32 0)
  store i8 98, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @strptr, i32 0, i32 1)
  store i8 99, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @strptr, i32 0, i32 2)
  store i8 100, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @strptr, i32 0, i32 3)
  ret i32 0
}
