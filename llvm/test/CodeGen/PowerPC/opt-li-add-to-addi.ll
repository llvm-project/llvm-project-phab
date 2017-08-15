; RUN: llc -verify-machineinstrs -mtriple=powerpc64-unknown-linux-gnu -mcpu=pwr8 < %s | FileCheck %s
; RUN: llc -verify-machineinstrs -mtriple=powerpc64le-unknown-linux-gnu -mcpu=pwr8 < %s | FileCheck %s

define i64 @testOptimizeLiAddToAddi(i64 %a) {
; CHECK-LABEL: testOptimizeLiAddToAddi:
; CHECK:    addi 3, 30, 2444
; CHECK:    bl callv
; CHECK:    addi 3, 30, 234
; CHECK:    bl call
; CHECK:    blr
entry:
  %cmp = icmp sgt i64 %a, 33
  br i1 %cmp, label %if.then, label %if.end

if.then:
  tail call void bitcast (void (...)* @callv to void ()*)()
  br label %if.end

if.end:
  %add.0 = phi i64 [ 234, %if.then ], [ 2444, %entry ]
  %add2 = add nsw i64 %add.0, %a
  %call = tail call i64 @call(i64 %add2)
  ret i64 %call
}

declare void @callv(...)

declare i64 @call(i64)
