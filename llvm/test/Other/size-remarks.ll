; RUN: opt < %s -inline -pass-remarks-analysis='size-info' -S 2>&1 \
; RUN:     | FileCheck %s
; CHECK: remark: <unknown>:0:0: Function Integration/Inlining:
; CHECK-SAME: IR instruction count changed from 15 to 29; Delta: 14
; RUN: opt < %s -inline -pass-remarks-analysis='size-info' -S \
; RUN: -pass-remarks-output=%t.yaml -o /dev/null 
; RUN: cat %t.yaml | FileCheck %s -check-prefix=YAML
; YAML: --- !Analysis
; YAML-NEXT: Pass:            size-info
; YAML-NEXT: Name:            IRSizeChange
; YAML-NEXT: Function:        foo
; YAML-NEXT: Args:            
; YAML-NEXT:  - Pass:            Function Integration/Inlining
; YAML-NEXT:  - String:          ': IR instruction count changed from '
; YAML-NEXT:  - IRInstrsBefore:  '15'
; YAML-NEXT:  - String:          ' to '
; YAML-NEXT:  - IRInstrsAfter:   '29'
; YAML-NEXT:  - String:          '; Delta: '
; YAML-NEXT:  - DeltaInstrCount: '14'

define i32 @foo(i32 %x, i32 %y) #0 {
entry:
  %x.addr = alloca i32, align 4
  %y.addr = alloca i32, align 4
  store i32 %x, i32* %x.addr, align 4
  store i32 %y, i32* %y.addr, align 4
  %0 = load i32, i32* %x.addr, align 4
  %1 = load i32, i32* %y.addr, align 4
  %add = add nsw i32 %0, %1
  ret i32 %add
}

define i32 @bar(i32 %j) #0 {
entry:
  %j.addr = alloca i32, align 4
  store i32 %j, i32* %j.addr, align 4
  %0 = load i32, i32* %j.addr, align 4
  %1 = load i32, i32* %j.addr, align 4
  %sub = sub nsw i32 %1, 2
  %call = call i32 @foo(i32 %0, i32 %sub)
  ret i32 %call
}