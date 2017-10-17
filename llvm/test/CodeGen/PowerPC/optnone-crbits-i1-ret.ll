; RUN: llc -verify-machineinstrs < %s -ppc-ignore-percent-prefix | FileCheck %s
target datalayout = "E-m:e-i64:64-n32:64"
target triple = "powerpc64-bgq-linux"

declare zeroext i1 @ri1()
declare void @se1()
declare void @se2()

define void @test() #0 {
entry:
  %b = call zeroext i1 @ri1()
  br label %next

; CHECK-LABEL: @test
; CHECK: bl ri1
; CHECK-NEXT: nop
; CHECK: andi. 3, 3, 1

next:
  br i1 %b, label %case1, label %case2

case1:
  call void @se1()
  br label %end

case2:
  call void @se2()
  br label %end

end:
  ret void

; CHECK: blr
}

attributes #0 = { noinline optnone }

