; RUN: llc -march=mipsel -mattr=+micromips -mcpu=mips32r2 -verify-machineinstrs < %s | FileCheck %s

; Function Attrs: nounwind
define i32 @fun(i32* %adr, i32 %val) {
entry:
; CHECK: swp
; CHECK: lwp
  %call1 =  call i32* @fun1()
  store i32 %val, i32* %adr, align 4
  ret i32 0
}

declare i32* @fun1()

