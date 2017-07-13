; RUN: llc < %s -mtriple=thumb-apple-darwin | FileCheck %s

declare void @consume_value(i32) #1

declare i32 @get_value(...) #1

declare void @consume_three_values(i32, i32, i32) #1

define void @should_not_spill() #0 {
  tail call void @consume_value(i32 42) #2
  %1 = tail call i32 (...) @get_value() #2
  %2 = tail call i32 (...) @get_value() #2
  %3 = tail call i32 (...) @get_value() #2
  tail call void @consume_value(i32 %1) #2
  tail call void @consume_value(i32 %2) #2
  tail call void @consume_value(i32 %3) #2
  tail call void @consume_value(i32 42) #2
  tail call void @consume_three_values(i32 %1, i32 %2, i32 %3) #2
  ret void
}

; CHECK: movs r0, #42
; CHECK: movs r0, #42
