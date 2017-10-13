; Ensure that IR count remarks in the pass manager work for every type of
; pass that we handle.

; First, make sure remarks work for CGSCC passes.
; RUN: opt < %s -inline -pass-remarks-analysis='size-info' -S 2>&1 \
; RUN:     | FileCheck %s -check-prefix=CGSCC
; CGSCC: remark: <unknown>:0:0: Function Integration/Inlining:
; CGSCC-SAME: IR instruction count changed from 17 to 31; Delta: 14
; RUN: opt < %s -inline -pass-remarks-analysis='size-info' -S \
; RUN: -pass-remarks-output=%t.cgscc.yaml -o /dev/null 
; RUN: cat %t.cgscc.yaml | FileCheck %s -check-prefix=CGSCC-YAML
; CGSCC-YAML: --- !Analysis
; CGSCC-YAML-NEXT: Pass:            size-info
; CGSCC-YAML-NEXT: Name:            IRSizeChange
; CGSCC-YAML-NEXT: Function:        baz
; CGSCC-YAML-NEXT: Args:            
; CGSCC-YAML-NEXT:  - Pass:            Function Integration/Inlining
; CGSCC-YAML-NEXT:  - String:          ': IR instruction count changed from '
; CGSCC-YAML-NEXT:  - IRInstrsBefore:  '17'
; CGSCC-YAML-NEXT:  - String:          ' to '
; CGSCC-YAML-NEXT:  - IRInstrsAfter:   '31'
; CGSCC-YAML-NEXT:  - String:          '; Delta: '
; CGSCC-YAML-NEXT:  - DeltaInstrCount: '14'

; Next, make sure it works for function passes.
; RUN: opt < %s -instcombine -pass-remarks-analysis='size-info' -S 2>&1 \
; RUN:     | FileCheck %s -check-prefix=FUNC
; FUNC: remark: <unknown>:0:0: Combine redundant instructions:
; FUNC-SAME: IR instruction count changed from 17 to 16; Delta: -1
; FUNC: remark: <unknown>:0:0: Combine redundant instructions:
; FUNC-SAME: IR instruction count changed from 16 to 10; Delta: -6
; FUNC: remark: <unknown>:0:0: Combine redundant instructions:
; FUNC-SAME: IR instruction count changed from 10 to 6; Delta: -4
; RUN: opt < %s -instcombine -pass-remarks-analysis='size-info' -S \
; RUN: -pass-remarks-output=%t.func.yaml -o /dev/null 
; RUN: cat %t.func.yaml | FileCheck %s -check-prefix=FUNC-YAML

; FUNC-YAML: --- !Analysis
; FUNC-YAML-NEXT: Pass:            size-info
; FUNC-YAML-NEXT: Name:            IRSizeChange
; FUNC-YAML-NEXT: Function:        baz
; FUNC-YAML-NEXT: Args:            
; FUNC-YAML-NEXT:  - Pass:            Combine redundant instructions
; FUNC-YAML-NEXT:  - String:          ': IR instruction count changed from '
; FUNC-YAML-NEXT:  - IRInstrsBefore:  '17'
; FUNC-YAML-NEXT:  - String:          ' to '
; FUNC-YAML-NEXT:  - IRInstrsAfter:   '16'
; FUNC-YAML-NEXT:  - String:          '; Delta: '
; FUNC-YAML-NEXT:  - DeltaInstrCount: '-1'

; FUNC-YAML: --- !Analysis
; FUNC-YAML-NEXT: Pass:            size-info
; FUNC-YAML-NEXT: Name:            IRSizeChange
; FUNC-YAML-NEXT: Function:        baz
; FUNC-YAML-NEXT: Args:            
; FUNC-YAML-NEXT:   - Pass:            Combine redundant instructions
; FUNC-YAML-NEXT:   - String:          ': IR instruction count changed from '
; FUNC-YAML-NEXT:   - IRInstrsBefore:  '16'
; FUNC-YAML-NEXT:   - String:          ' to '
; FUNC-YAML-NEXT:   - IRInstrsAfter:   '10'
; FUNC-YAML-NEXT:   - String:          '; Delta: '
; FUNC-YAML-NEXT:   - DeltaInstrCount: '-6'

; FUNC-YAML: --- !Analysis
; FUNC-YAML-NEXT: Pass:            size-info
; FUNC-YAML-NEXT: Name:            IRSizeChange
; FUNC-YAML-NEXT: Function:        baz
; FUNC-YAML-NEXT: Args:            
; FUNC-YAML-NEXT:  - Pass:            Combine redundant instructions
; FUNC-YAML-NEXT:  - String:          ': IR instruction count changed from '
; FUNC-YAML-NEXT:  - IRInstrsBefore:  '10'
; FUNC-YAML-NEXT:  - String:          ' to '
; FUNC-YAML-NEXT:  - IRInstrsAfter:   '6'
; FUNC-YAML-NEXT:  - String:          '; Delta: '
; FUNC-YAML-NEXT:  - DeltaInstrCount: '-4'

; Make sure it works for module passes.
; RUN: opt < %s -gvn -pass-remarks-analysis='size-info' -S 2>&1 \
; RUN:     | FileCheck %s -check-prefix=MODULE
; MODULE: remark: <unknown>:0:0: Global Value Numbering:
; MODULE-SAME: IR instruction count changed from 17 to 15; Delta: -2
; MODULE: remark: <unknown>:0:0: Global Value Numbering:
; MODULE-SAME: IR instruction count changed from 15 to 13; Delta: -2
; RUN: opt < %s -gvn -pass-remarks-analysis='size-info' -S \
; RUN: -pass-remarks-output=%t.module.yaml -o /dev/null 
; RUN: cat %t.module.yaml | FileCheck %s -check-prefix=MODULE-YAML
; MODULE-YAML: --- !Analysis
; MODULE-YAML-NEXT: Pass:            size-info
; MODULE-YAML-NEXT: Name:            IRSizeChange
; MODULE-YAML-NEXT: Function:        baz
; MODULE-YAML-NEXT: Args:            
; MODULE-YAML-NEXT:   - Pass:            Global Value Numbering
; MODULE-YAML-NEXT:   - String:          ': IR instruction count changed from '
; MODULE-YAML-NEXT:   - IRInstrsBefore:  '17'
; MODULE-YAML-NEXT:   - String:          ' to '
; MODULE-YAML-NEXT:   - IRInstrsAfter:   '15'
; MODULE-YAML-NEXT:   - String:          '; Delta: '
; MODULE-YAML-NEXT:   - DeltaInstrCount: '-2'
; MODULE-YAML: --- !Analysis
; MODULE-YAML-NEXT: Pass:            size-info
; MODULE-YAML-NEXT: Name:            IRSizeChange
; MODULE-YAML-NEXT: Function:        baz
; MODULE-YAML-NEXT: Args:            
; MODULE-YAML-NEXT:  - Pass:            Global Value Numbering
; MODULE-YAML-NEXT:  - String:          ': IR instruction count changed from '
; MODULE-YAML-NEXT:  - IRInstrsBefore:  '15'
; MODULE-YAML-NEXT:  - String:          ' to '
; MODULE-YAML-NEXT:  - IRInstrsAfter:   '13'
; MODULE-YAML-NEXT:  - String:          '; Delta: '
; MODULE-YAML-NEXT:  - DeltaInstrCount: '-2'

; Make sure it works for basic block passes.
; RUN: opt < %s -dce -pass-remarks-analysis='size-info' -S 2>&1 \
; RUN:     | FileCheck %s -check-prefix=BB
; BB: remark: <unknown>:0:0: Dead Code Elimination:
; BB-SAME: IR instruction count changed from 17 to 16; Delta: -1
; RUN: opt < %s -dce -pass-remarks-analysis='size-info' -S \
; RUN: -pass-remarks-output=%t.bb.yaml -o /dev/null 
; RUN: cat %t.bb.yaml | FileCheck %s -check-prefix=BB-YAML
; BB-YAML: --- !Analysis
; BB-YAML-NEXT: Pass:            size-info
; BB-YAML-NEXT: Name:            IRSizeChange
; BB-YAML-NEXT: Function:        baz
; BB-YAML-NEXT: Args:            
; BB-YAML-NEXT:   - Pass:            Dead Code Elimination
; BB-YAML-NEXT:   - String:          ': IR instruction count changed from '
; BB-YAML-NEXT:   - IRInstrsBefore:  '17'
; BB-YAML-NEXT:   - String:          ' to '
; BB-YAML-NEXT:   - IRInstrsAfter:   '16'
; BB-YAML-NEXT:   - String:          '; Delta: '
; BB-YAML-NEXT:   - DeltaInstrCount: '-1'

define void @baz() #0 {
  %x.addr = alloca i32, align 4
  ret void
}

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