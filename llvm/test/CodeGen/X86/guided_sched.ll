; NOTE: Both functions must emit same instruction schedule with -guided-src 
; RUN: llc -pre-RA-sched=guided-src -mtriple=x86_64-unknown-linux-gnu -mattr=+lzcnt < %s | FileCheck %s 

; CHECK-LABEL: clz_i128:
; CHECK-NOT: testq
; CHECK    : cmovael
define i32 @clz_i128(i64, i64)  {
  %3 = tail call i64 @llvm.ctlz.i64(i64 %1, i1 false)
  %4 = tail call i64 @llvm.ctlz.i64(i64 %0, i1 false)
  %5 = icmp ne i64 %0, 0
  %6 = select i1 %5, i64 0, i64 %3
  %7 = add nuw nsw i64 %6, %4
  %8 = trunc i64 %7 to i32
  ret i32 %8
}

; CHECK-LABEL: clz_i128_swap:
; CHECK-NOT: testq
; CHECK    : cmovael
define i32 @clz_i128_swap(i64, i64)  {
  %3 = tail call i64 @llvm.ctlz.i64(i64 %0, i1 false) ;   <-- SWAP
  %4 = tail call i64 @llvm.ctlz.i64(i64 %1, i1 false) ;   <-- SWAP
  %5 = icmp ne i64 %0, 0
  %6 = select i1 %5, i64 0, i64 %4
  %7 = add nuw nsw i64 %6, %3
  %8 = trunc i64 %7 to i32
  ret i32 %8
}
declare i64 @llvm.ctlz.i64(i64, i1)

