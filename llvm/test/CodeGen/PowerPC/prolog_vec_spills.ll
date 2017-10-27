; RUN: llc -verify-machineinstrs -mtriple=powerpc64le-unknown-linux-gnu -ppc-enable-pe-vector-spills -mcpu=pwr9 < %s | FileCheck %s -check-prefix=CHECK

define signext i32 @test1(i32 signext %a, i32 signext %b) #0 {
entry:
; CHECK: mtvsrd [[REG1:[0-9]+]], 14
; CHECK: mtvsrd [[REG2:[0-9]+]], 15
; CHECK: mtvsrd [[REG3:[0-9]+]], 16
; CHECK: mffprd   16, [[REG3]]
; CHECK: mffprd   15, [[REG2]]
; CHECK: mffprd   14, [[REG1]]
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  %dst = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  call void asm sideeffect "", "~{v20},~{f14},~{f0}"()
  %0 = load i32, i32* %a.addr, align 4
  %1 = load i32, i32* %b.addr, align 4
  %2 = call i32 asm "add $0, $1, $2", "=r,r,r,~{r14},~{r15},~{r16}"(i32 %0, i32 %1)
  store i32 %2, i32* %dst, align 4
  %3 = load i32, i32* %dst, align 4
  ret i32 %3
}

attributes #0 = { nounwind }
