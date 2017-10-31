; RUN: opt < %s -S -early-cse | FileCheck %s

; Check that load (with selected address) is deleted

; CHECK-LABEL: @min(
; CHECK: load i32
; CHECK: load i32
; CHECK-NOT: load i32

; Function Attrs: norecurse nounwind readonly uwtable
define i32 @min(i32* nocapture readonly, i32* nocapture readonly, i32) local_unnamed_addr {
  %4 = sext i32 %2 to i64
  %5 = getelementptr inbounds i32, i32* %0, i64 %4
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds i32, i32* %1, i64 %4
  %8 = load i32, i32* %7, align 4
  %9 = icmp slt i32 %6, %8
  %10 = select i1 %9, i32* %0, i32* %1
  %11 = getelementptr inbounds i32, i32* %10, i64 %4
  %12 = load i32, i32* %11, align 4
  ret i32 %12
}
