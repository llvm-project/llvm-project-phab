; SROA fails to rewrite allocs but does rewrite some phis and delete
; dead instructions. Ensure that this invalidates analyses required
; for other passes.
; RUN: opt < %s -passes=bdce,sroa,bdce -o %t -debug-pass-manager 2>&1 | FileCheck %s
; CHECK: Running pass: BDCEPass on H
; CHECK: Running analysis: DemandedBitsAnalysis on H
; CHECK: Running pass: SROA on H
; CHECK: Invalidating all non-preserved analyses for: H
; CHECK: Invalidating analysis: DemandedBitsAnalysis on H
; CHECK: Running pass: BDCEPass on H
; CHECK: Running analysis: DemandedBitsAnalysis on H
; CHECK: Finished llvm::Function pass manager run.

; ModuleID = 'bugpoint-reduced-simplified.bc'
source_filename = "bugpoint-output-7a27a2b.bc"
target datalayout = "e-m:e-i64:64-n32:64"
target triple = "powerpc64le-grtev4-linux-gnu"

%"struct.X" = type { %"class.B", %"class.B" }
%"class.B" = type { %"struct.J2" }
%"struct.J2" = type { %"struct.K1" }
%"struct.K1" = type { i64 }
%"struct.K0" = type { i8 }
%class.b = type { %"class.L" }
%"class.L" = type { %"class.M" }
%"class.M" = type { %"struct.N", i64, %union.anon }
%"struct.N" = type { i8* }
%union.anon = type { i64, [8 x i8] }
%"class.I" = type { i64 }
%"class.F" = type { %"struct.A" }
%"struct.A" = type <{ i8*, i8*, i8*, i8*, i8* (i32, i8*, i8*, i8*)*, void (i8*)*, i8, i8, i8, [5 x i8], i1 ()*, void ()*, i8*, i8*, i64, %"struct.J2", %"struct.X"*, %"struct.J1", [7 x i8] }>
%"struct.J1" = type { %"struct.J3" }
%"struct.J3" = type { %"struct.K0" }

@C = external global { { i8*, i8*, i8*, i8*, i8* (i32, i8*, i8*, i8*)*, void (i8*)*, i8, i8, i8, i1 ()*, void ()*, i8*, i8*, i64, { i64 }, %"struct.X"*, { %"struct.K0" } } }, align 8
@.str.14 = external hidden unnamed_addr constant [5 x i8], align 1

declare void @D(%class.b* sret, %class.b* dereferenceable(32)) local_unnamed_addr #0

; Function Attrs: nounwind
define hidden fastcc void @H(%"class.I"* noalias nocapture readnone, [2 x i64]) unnamed_addr #1 !prof !1 {
  %3 = alloca %class.b, align 8
  %.sroa.0 = alloca i64, align 8
  store i64 0, i64* %.sroa.0, align 8
  %4 = extractvalue [2 x i64] %1, 1
  switch i64 %4, label %6 [
    i64 4, label %37
    i64 5, label %5
  ]

; <label>:5:                                      ; preds = %2
  %.sroa.0.0..sroa_cast3 = bitcast i64* %.sroa.0 to i8**
  br label %12

; <label>:6:                                      ; preds = %2
  %7 = icmp ugt i64 %4, 5
  %.sroa.0.0..sroa_cast5 = bitcast i64* %.sroa.0 to i8**
  br i1 %7, label %8, label %12

; <label>:8:                                      ; preds = %6
  %9 = load i8, i8* inttoptr (i64 4 to i8*), align 4
  %10 = icmp eq i8 %9, 47
  %11 = select i1 %10, i64 5, i64 4
  br label %12

; <label>:12:                                     ; preds = %5, %8, %6
  %13 = phi i8** [ %.sroa.0.0..sroa_cast3, %5 ], [ %.sroa.0.0..sroa_cast5, %8 ], [ %.sroa.0.0..sroa_cast5, %6 ]
  %14 = phi i64 [ 4, %5 ], [ %11, %8 ], [ 4, %6 ]
  %15 = icmp ne i64 %4, 0
  %16 = icmp ugt i64 %4, %14
  %17 = and i1 %15, %16
  br i1 %17, label %18, label %a.exit, !prof !3

; <label>:18:                                     ; preds = %12
  %19 = tail call i8* @memchr(i8* undef, i32 signext undef, i64 undef) #7, !prof !2
  %20 = icmp eq i8* %19, null
  %21 = sext i1 %20 to i64
  br label %a.exit

a.exit:              ; preds = %12, %18
  %22 = phi i64 [ -1, %12 ], [ %21, %18 ]
  %23 = load i8*, i8** %13, align 8
  %24 = sub nsw i64 %22, %14
  %25 = bitcast %class.b* %3 to i8*
  call void @llvm.lifetime.start.p0i8(i64 32, i8* nonnull %25)
  %26 = icmp ult i64 %24, 2
  br i1 %26, label %G.exit, label %27

; <label>:27:                                     ; preds = %a.exit
  %28 = getelementptr inbounds i8, i8* %23, i64 undef
  %29 = icmp eq i8* %28, null
  br i1 %29, label %30, label %31, !prof !4

; <label>:30:                                     ; preds = %27
  tail call void @llvm.trap() #6
  unreachable

; <label>:31:                                     ; preds = %27
  call void @D(%class.b* nonnull sret %3, %class.b* nonnull dereferenceable(32) undef) #6
  %32 = call zeroext i1 @E(%"class.F"* dereferenceable(120) bitcast ({ { i8*, i8*, i8*, i8*, i8* (i32, i8*, i8*, i8*)*, void (i8*)*, i8, i8, i8, i1 ()*, void ()*, i8*, i8*, i64, { i64 }, %"struct.X"*, { %"struct.K0" } } }* @C to %"class.F"*)) #6
  br i1 %32, label %33, label %G.exit

; <label>:33:                                     ; preds = %31
  %34 = call signext i32 @memcmp(i8* undef, i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.14, i64 0, i64 0), i64 4) #7, !prof !2
  %35 = icmp eq i32 %34, 0
  br i1 %35, label %36, label %G.exit

; <label>:36:                                     ; preds = %33
  call fastcc void @H(%"class.I"* noalias undef, [2 x i64] undef) #6
  br label %G.exit

G.exit: ; preds = %36, %33, %31, %a.exit
  call void @llvm.lifetime.end.p0i8(i64 32, i8* nonnull %25)
  br label %37

; <label>:37:                                     ; preds = %G.exit, %2
  ret void
}

; Function Attrs: nounwind readonly
declare signext i32 @memcmp(i8* nocapture, i8* nocapture, i64) local_unnamed_addr #2

; Function Attrs: nounwind
declare zeroext i1 @E(%"class.F"* dereferenceable(120)) local_unnamed_addr #1

; Function Attrs: nounwind readonly
declare i8* @memchr(i8*, i32 signext, i64) local_unnamed_addr #2

; Function Attrs: noreturn nounwind
declare void @llvm.trap() #4

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #5

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #5

!1 = !{!"function_entry_count", i64 0}
!2 = !{!"branch_weights", i64 0}
!3 = !{!"branch_weights", i32 -1305080176, i32 47789939}
!4 = !{!"branch_weights", i32 97, i32 16829}
