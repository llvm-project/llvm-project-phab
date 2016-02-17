; RUN: opt < %s -S -licm -loop-unswitch | FileCheck %s

; Function Attrs: nounwind uwtable
define i32 @main() #1 {
entry:
; CHECK: entry:
; CHECK-NEXT: br label %third.body
  br label %first.header

first.header:
; CHECK: first.header:
; CHECK-NEXT: br label %third.body.licm_exit
  br label %second.header

second.header:
; CHECK: second.header:
; CHECK-NEXT: br label %second.exiting
  br label %third.body

third.body:
; CHECK: third.body:
  %xor1.i = phi i32 [ 0, %second.header ], [ %xor1, %third.body ]
  %xor1 = xor i32 %xor1.i, 1234567
; CHECK: br i1 false, label %third.body, label %third.body.licm_exit
  br i1 undef, label %third.body, label %second.exiting

second.exiting:
; CHECK: second.exiting:
  %xor1.lcssa = phi i32 [ %xor1, %third.body ]
  %call1 = tail call i32 (i32) @putchar(i32 %xor1.lcssa)
; CHECK: br i1 false, label %second.header, label %first.exiting
  br i1 undef, label %second.header, label %first.exiting

first.exiting:
  %xor1.lcssa2 = phi i32 [ %xor1.lcssa, %second.exiting ]
  %call2 = tail call i32 (i32) @putchar(i32 %xor1.lcssa2)
  br i1 false, label %first.header, label %first.exit

first.exit:
  ret i32 0

; CHECK: third.body.licm_exit:
; CHECK-NEXT: br label %second.header

; CHECK: third.body.licm_exit1:
; CHECK-NEXT: phi i32 [ %xor1, %third.body ]
; CHECK-NEXT br label %first.header
}

; Function Attrs: nounwind
declare i32 @putchar(i32) #2
