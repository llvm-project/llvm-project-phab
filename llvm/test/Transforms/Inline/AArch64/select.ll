; REQUIRES: asserts
; RUN: opt -inline -mtriple=aarch64--linux-gnu -S -debug-only=inline-cost < %s 2>&1 | FileCheck %s

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64--linux-gnu"

define i32 @outer1(i1 %cond) {
  %C = call i32 @inner1(i1 %cond, i32 1)
  ret i32 %C
}

; CHECK: Analyzing call of inner1
; CHECK: NumInstructionsSimplified: 2
; CHECK: NumInstructions: 2
define i32 @inner1(i1 %cond, i32 %val) {
  %select = select i1 %cond, i32 1, i32 %val       ; Simplified to 1
  ret i32 %select                                  ; Simplifies to ret i32 1
}


define i32 @outer2(i32 %val) {
  %C = call i32 @inner2(i1 true, i32 %val)
  ret i32 %C
}

; CHECK: Analyzing call of inner2
; CHECK: NumInstructionsSimplified: 2
; CHECK: NumInstructions: 2
define i32 @inner2(i1 %cond, i32 %val) {
  %select = select i1 %cond, i32 1, i32 %val       ; Simplifies to 1
  ret i32 %select                                  ; Simplifies to ret i32 1
}


define i32 @outer3(i32 %val) {
  %C = call i32 @inner3(i1 false, i32 %val)
  ret i32 %C
}

; CHECK: Analyzing call of inner3
; CHECK: NumInstructionsSimplified: 2
; CHECK: NumInstructions: 2
define i32 @inner3(i1 %cond, i32 %val) {
  %select = select i1 %cond, i32 %val, i32 -1      ; Simplifies to -1
  ret i32 %select                                  ; Simplifies to ret i32 -1
}


define i32 @outer4() {
  %C = call i32 @inner4(i1 true, i32 1, i32 -1)
  ret i32 %C
}

; CHECK: Analyzing call of inner4
; CHECK: NumInstructionsSimplified: 2
; CHECK: NumInstructions: 2
define i32 @inner4(i1 %cond, i32 %val1, i32 %val2) {
  %select = select i1 %cond, i32 %val1, i32 %val2  ; Simplifies to 1
  ret i32 %select                                  ; Simplifies to ret i32 1
}


define i1 @outer5() {
  %C = call i1 @inner5(i1 true, i1 true, i1 false)
  ret i1 %C
}

; CHECK: Analyzing call of inner5
; CHECK: NumInstructionsSimplified: 3
; CHECK: NumInstructions: 3
define i1 @inner5(i1 %cond, i1 %val1, i1 %val2) {
  %select = select i1 %cond, i1 %val1, i1 %val2    ; Simplifies to true
  br i1 %select, label %end, label %isfalse        ; Simplifies to br label %end

isfalse:                                           ; This block is unreachable once inlined
  call void @dead()
  call void @dead()
  call void @dead()
  call void @dead()
  call void @dead()
  br label %end

end:
  ret i1 %select                                   ; Simplifies to ret i1 true
}

declare void @dead()


define i32 @outer6(i1 %cond) {
  %A = alloca i32
  %C = call i32 @inner6(i1 %cond, i32* %A)
  ret i32 %C
}

; CHECK: Analyzing call of inner6
; CHECK: NumInstructionsSimplified: 6
; CHECK: NumInstructions: 6
define i32 @inner6(i1 %cond, i32* %ptr) {
  %G1 = getelementptr inbounds i32, i32* %ptr, i32 1
  %G2 = getelementptr inbounds i32, i32* %G1, i32 1
  %G3 = getelementptr inbounds i32, i32* %ptr, i32 2
  %select = select i1 %cond, i32* %G2, i32* %G3    ; Simplified to %A[2]
  %load = load i32, i32* %select                   ; SROA'ed
  ret i32 %load                                    ; Simplified
}


define i32 @outer7(i32* %ptr) {
  %A = alloca i32
  %C = call i32 @inner7(i1 true, i32* %A, i32* %ptr)
  ret i32 %C
}

; CHECK: Analyzing call of inner7
; CHECK: NumInstructionsSimplified: 3
; CHECK: NumInstructions: 3
define i32 @inner7(i1 %cond, i32* %p1, i32* %p2) {
  %select = select i1 %cond, i32* %p1, i32* %p2      ; Simplifies to %A
  %load = load i32, i32* %select                    ; SROA'ed
  ret i32 %load                                    ; Simplified
}


define i32 @outer8(i32* %ptr) {
  %A = alloca i32
  %C = call i32 @inner8(i1 false, i32* %ptr, i32* %A)
  ret i32 %C
}

; CHECK: Analyzing call of inner8
; CHECK: NumInstructionsSimplified: 3
; CHECK: NumInstructions: 3
define i32 @inner8(i1 %cond, i32* %p1, i32* %p2) {
  %select = select i1 %cond, i32* %p1, i32* %p2      ; Simplifies to %A
  %load = load i32, i32* %select                    ; SROA'ed
  ret i32 %load                                    ; Simplified
}


define <2 x i32> @outer9(<2 x i32> %val) {
  %C = call <2 x i32> @inner9(<2 x i1> <i1 true, i1 true>, <2 x i32> %val)
  ret <2 x i32> %C
}

; CHECK: Analyzing call of inner9
; CHECK: NumInstructionsSimplified: 2
; CHECK: NumInstructions: 2
define <2 x i32> @inner9(<2 x i1> %cond, <2 x i32> %val) {
  %select = select <2 x i1> %cond, <2 x i32> <i32 1, i32 1>, <2 x i32> %val              ; Simplifies to <1, 1>
  ret <2 x i32> %select                                                                  ; Simplifies to ret <2 x i32> <1, 1>
}


define <2 x i32> @outer10(<2 x i32> %val) {
  %C = call <2 x i32> @inner10(<2 x i1> <i1 false, i1 false>, <2 x i32> %val)
  ret <2 x i32> %C
}

; CHECK: Analyzing call of inner10
; CHECK: NumInstructionsSimplified: 2
; CHECK: NumInstructions: 2
define <2 x i32> @inner10(<2 x i1> %cond, <2 x i32> %val) {
  %select = select <2 x i1> %cond, <2 x i32> <i32 1, i32 1>, < 2 x i32> %val             ; Simplifies to <-1, -1>
  ret <2 x i32> %select                                                                  ; Simplifies to ret <2 x i32> <-1, -1>
}


define <2 x i32> @outer11() {
  %C = call <2 x i32> @inner11(<2 x i1> <i1 true, i1 false>)
  ret <2 x i32> %C
}

; CHECK: Analyzing call of inner11
; CHECK: NumInstructionsSimplified: 2
; CHECK: NumInstructions: 2
define <2 x i32> @inner11(<2 x i1> %cond) {
  %select = select <2 x i1> %cond, <2 x i32> <i32 1, i32 1>, < 2 x i32> <i32 -1, i32 -1> ; Simplifies to <1, -1>
  ret <2 x i32> %select                                                                  ; Simplifies to ret <2 x i32> <1, -1>
}
