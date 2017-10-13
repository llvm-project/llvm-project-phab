; RUN: opt < %s -instcombine -inline -inline-threshold=0

; CHECK: define i32 @main() {
; CHECK:   %1 = call i32 @rand()
; CHECK:   %2 = call i32 @rand()
; CHECK:   %3 = add nsw i32 %1, %2
; CHECK:   ret i32 %3
; CHECK: }

define internal i32 @foo() #0 {
  %1 = call i32 @rand() #2
  %2 = call i32 @rand() #2
  %3 = add nsw i32 %1, %2
  ret i32 %3
}

declare i32 @rand() #1

define i32 @main() #0 {
  %1 = tail call i32 (...) bitcast (i32 ()* @foo to i32 (...)*)() #2
  ret i32 %1
}
