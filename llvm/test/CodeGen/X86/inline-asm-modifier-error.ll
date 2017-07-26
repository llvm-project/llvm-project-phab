; RUN: not llc -march x86 < %s 2> %t
; RUN: FileCheck %s < %t

; 'H' modifier, when applied on a non-memory operand
; CHECK: error: Cannot use 'H' modifier on a non-offsettable memory reference: 'movl $0, ${1:H}'

define void @foo() local_unnamed_addr #0 {
entry:
  tail call void asm sideeffect "movl $0, ${1:H}", "i,i,~{dirflag},~{fpsr},~{flags}"(i32 1, i32 2)
  ret void
}

