; RUN: opt -delinearize -analyze < %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK:      Inst:  store i64 -281474976710602, i64* %19
; CHECK-NEXT: In Loop with Header: Block #6
; CHECK-NEXT: AccessFunction: (8 * (sext i32 {{[{][{]}}0,+,(trunc i64 %4 to i32)}<%"Block #3">,+,1}<%"Block #6"> to i64))
; CHECK-NEXT: Base offset: %18
; CHECK-NEXT: ArrayDecl[UnknownSize][%4] with elements of 8 bytes.
; CHECK-NEXT: ArrayRef[(sext i32 {0,+,1}<%"Block #3"> to i64)][{0,+,1}<nuw><nsw><%"Block #6">]

; CHECK:      Inst:  store i64 -281474976710602, i64* %19
; CHECK-NEXT: In Loop with Header: Block #3
; CHECK-NEXT: AccessFunction: (8 * (sext i32 {(-1 + (trunc i64 %4 to i32)),+,(trunc i64 %4 to i32)}<%"Block #3"> to i64))
; CHECK-NEXT: Base offset: %18
; CHECK-NEXT: ArrayDecl[UnknownSize][%4] with elements of 8 bytes.
; CHECK-NEXT: ArrayRef[(sext i32 {1,+,1}<%"Block #3"> to i64)][-1]

define i64 @jsBody_test() #0 {
Prologue:
  %0 = call i8* @llvm.frameaddress(i32 0)
  %1 = ptrtoint i8* %0 to i64
  %2 = add i64 %1, 56
  %3 = inttoptr i64 %2 to i64*
  %4 = load i64, i64* %3
  %5 = add i64 %1, 48
  %6 = inttoptr i64 %5 to i64*
  %7 = load i64, i64* %6
  %8 = trunc i64 %4 to i32
  %9 = icmp slt i32 0, %8
  %10 = add i64 %7, 8
  %11 = inttoptr i64 %10 to i64*
  %12 = load i64, i64* %11
  %13 = add i64 %12, 0
  br label %"Block #3"

"Block #1":                                       ; preds = %"Block #9"
  br label %"Block #3"

"Block #3":                                       ; preds = %"Block #1", %Prologue
  %.01 = phi i32 [ 0, %Prologue ], [ %22, %"Block #1" ]
  br i1 %9, label %"Block #4", label %"Block #9"

"Block #4":                                       ; preds = %"Block #3"
  %14 = call { i32, i1 } @llvm.smul.with.overflow.i32(i32 %8, i32 %.01)
  %15 = extractvalue { i32, i1 } %14, 0
  br label %"Block #6"

"Block #5":                                       ; preds = %"Block #6"
  br label %"Block #6"

"Block #6":                                       ; preds = %"Block #5", %"Block #4"
  %.0 = phi i32 [ 0, %"Block #4" ], [ %20, %"Block #5" ]
  %16 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %15, i32 %.0)
  %17 = extractvalue { i32, i1 } %16, 0
  %18 = inttoptr i64 %13 to [10000000 x i64]*
  %19 = getelementptr [10000000 x i64], [10000000 x i64]* %18, i32 0, i32 %17
  store i64 -281474976710602, i64* %19
  %20 = add i32 %.0, 1
  %21 = icmp slt i32 %20, %8
  br i1 %21, label %"Block #5", label %"Block #9"

"Block #9":                                       ; preds = %"Block #6", %"Block #3"
  %22 = add i32 %.01, 1
  %23 = icmp slt i32 %22, 1000
  br i1 %23, label %"Block #1", label %"Block #10"

"Block #10":                                      ; preds = %"Block #9"
  ret i64 10
}

; Function Attrs: nounwind readnone
declare i8* @llvm.frameaddress(i32) #1

; Function Attrs: nounwind readnone speculatable
declare { i32, i1 } @llvm.smul.with.overflow.i32(i32, i32) #2

; Function Attrs: nounwind readnone speculatable
declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32) #2

attributes #0 = { "target-features"="-avx" }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind readnone speculatable }
