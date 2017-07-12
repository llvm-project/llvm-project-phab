; RUN: llc < %s -mtriple=thumbv6m-eabi -verify-machineinstrs | FileCheck %s

define i32 @uadd_overflow(i32 %a, i32 %b) #0 {
  %sadd = tail call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %sadd, 1
  %2 = zext i1 %1 to i32
  ret i32 %2

  ; CHECK-LABEL: uadd_overflow:
  ; CHECK: movs    r[[R2:[0-9]+]], #0
  ; CHECK: adds    r[[R0:[0-9]+]], r[[R0]], r[[R1:[0-9]+]]
  ; CHECK: adcs    r[[R2]], r[[R2]]
  ; CHECK: mov     r[[R0]], r[[R2]]
}


define i32 @sadd_overflow(i32 %a, i32 %b) #0 {
  %sadd = tail call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %sadd, 1
  %2 = zext i1 %1 to i32
  ret i32 %2

  ; CHECK-LABEL: sadd_overflow:
  ; CHECK: mov  r[[R2:[0-9]+]], r[[R0:[0-9]+]]
  ; CHECK: adds r[[R3:[0-9]+]], r[[R2]], r[[R1:[0-9]+]]
  ; CHECK: movs r[[R0]], #0
  ; CHECK: movs r[[R1]], #1
  ; CHECK: cmp  r[[R3]], r[[R2]]
  ; CHECK: bvc  .L[[LABEL:.*]]
  ; CHECK: mov  r[[R0]], r[[R1]]
  ; CHECK: .L[[LABEL]]:
}

define i32 @usub_overflow(i32 %a, i32 %b) #0 {
  %sadd = tail call { i32, i1 } @llvm.usub.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %sadd, 1
  %2 = zext i1 %1 to i32
  ret i32 %2

  ; CHECK-LABEL: usub_overflow:
  ; CHECK: movs    r[[R3:[0-9]+]], #0
  ; CHECK: movs    r[[R2:[0-9]+]], #1
  ; CHECK: subs    r[[R0:[0-9]+]], r[[R0]], r[[R1:[0-9]+]]
  ; CHECK: sbcs    r[[R2]], r[[R3]]
  ; CHECK: mov     r[[R0]], r[[R2]]
}

define i32 @ssub_overflow(i32 %a, i32 %b) #0 {
  %sadd = tail call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %sadd, 1
  %2 = zext i1 %1 to i32
  ret i32 %2

  ; CHECK: mov     r[[R2:[0-9]+]], r[[R0:[0-9]+]]
  ; CHECK: movs    r[[R0]], #0
  ; CHECK: movs    r[[R3:[0-9]+]], #1
  ; CHECK: cmp     r[[R2]], r[[R1:[0-9]+]]
  ; CHECK: bvc     .L[[LABEL:.*]]
  ; CHECK: mov     r[[R0]], r[[R3]]
  ; CHECK: .L[[LABEL]]:
}

declare { i32, i1 } @llvm.uadd.with.overflow.i32(i32, i32) #1
declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32) #2
declare { i32, i1 } @llvm.usub.with.overflow.i32(i32, i32) #3
declare { i32, i1 } @llvm.ssub.with.overflow.i32(i32, i32) #4
