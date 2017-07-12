; RUN: llc < %s -mtriple=thumbv7-eabi -verify-machineinstrs | FileCheck %s

define i32 @uadd_overflow(i32 %a, i32 %b) #0 {
  %sadd = tail call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %sadd, 1
  %2 = zext i1 %1 to i32
  ret i32 %2

  ; CHECK-LABEL: uadd_overflow:
  ; CHECK: adds  r[[R0:[0-9]+]], r[[R0]], r[[R1:[0-9]+]]
  ; CHECK: mov.w r[[R2:[0-9]+]], #0
  ; CHECK: adc   r[[R0]], r[[R2]], #0
}


define i32 @sadd_overflow(i32 %a, i32 %b) #0 {
  %sadd = tail call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %sadd, 1
  %2 = zext i1 %1 to i32
  ret i32 %2

  ; CHECK: adds  r[[R2:[0-9]+]], r[[R0:[0-9]+]], r[[R1:[0-9]+]]
  ; CHECK: movs  r[[R1]], #1
  ; CHECK: cmp   r[[R2]], r[[R0]]
  ; CHECK: it    vc
  ; CHECK: movvc r[[R1]], #0
  ; CHECK: mov   r[[R0]], r[[R1]]
}

define i32 @usub_overflow(i32 %a, i32 %b) #0 {
  %sadd = tail call { i32, i1 } @llvm.usub.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %sadd, 1
  %2 = zext i1 %1 to i32
  ret i32 %2

  ; CHECK-LABEL: usub_overflow:
  ; CHECK: subs  r[[R0:[0-9]+]], r[[R0]], r[[R1:[0-9]+]]
  ; CHECK: mov.w r[[R2:[0-9]+]], #1
  ; CHECK: sbc   r[[R0]], r[[R2]], #0
}

define i32 @ssub_overflow(i32 %a, i32 %b) #0 {
  %sadd = tail call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %sadd, 1
  %2 = zext i1 %1 to i32
  ret i32 %2

  ; CHECK-LABEL: ssub_overflow:
  ; CHECK: movs  r[[R2:[0-9]+]], #1
  ; CHECK: cmp   r[[R0:[0-9]+]], r[[R1:[0-9]+]]
  ; CHECK: it    vc
  ; CHECK: movvc r[[R2]], #0
  ; CHECK: mov   r[[R0]], r[[R2]]
}

declare { i32, i1 } @llvm.uadd.with.overflow.i32(i32, i32) #1
declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32) #2
declare { i32, i1 } @llvm.usub.with.overflow.i32(i32, i32) #3
declare { i32, i1 } @llvm.ssub.with.overflow.i32(i32, i32) #4
