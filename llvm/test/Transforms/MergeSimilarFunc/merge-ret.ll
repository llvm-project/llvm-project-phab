; Check that the ret instructions are merged correctly. A bug caused
; an incorrect merge and a verifier failure for this input.
;
; RUN: opt -S -mergesimilarfunc < %s | FileCheck %s
;
; CHECK-LABEL: define internal %0* @LLVMGetReturnType__merged
; CHECK: phi %0* [
; CHECK-NEXT: ret %0*
; CHECK-NEXT: }
;

target datalayout = "e-m:e-p:32:32:32-a:0-n16:32-i64:64:64-i32:32:32-i16:16:16-i1:8:8-f32:32:32-f64:64:64-v32:32:32-v64:64:64-v512:512:512-v1024:1024:1024-v2048:2048:2048"

%0 = type opaque
%1 = type { %2*, i32, i32, %1** }
%2 = type { %3* }
%3 = type opaque
%4 = type { %1 }
%5 = type opaque
%6 = type { i32 (...)**, %1*, %7*, i8, i8, i16, i32 }
%7 = type { %6*, %7*, %8 }
%8 = type { i32 }
%9 = type { %10, %1*, i32, %12, i32, i8, %18* }
%10 = type { %11 }
%11 = type { %6 }
%12 = type { %13 }
%13 = type { %14 }
%14 = type { %15 }
%15 = type { %16 }
%16 = type { %17 }
%17 = type { i32, i32, i8* }
%18 = type <{ %2*, %19, %30, %70, %79, %87, %12, %93*, %94, %98, %12, %12, %12, i8*, %102, i8, [3 x i8] }>
%19 = type { %20 }
%20 = type { %21, %26* }
%21 = type { %22 }
%22 = type { %23 }
%23 = type { %24 }
%24 = type { %25, %26* }
%25 = type { %26* }
%26 = type <{ %27, [3 x i8], %24, i8, [3 x i8] }>
%27 = type <{ %9, %12, %28*, %12, %12, i8 }>
%28 = type <{ %29*, i8, [3 x i8] }>
%29 = type opaque
%30 = type { %31 }
%31 = type { %32, %37* }
%32 = type { %33 }
%33 = type { %34 }
%34 = type { %35 }
%35 = type { %36, %37* }
%36 = type { %37* }
%37 = type { %27, %35, %38, %60, %93*, %68, %4* }
%38 = type { %39 }
%39 = type { %40, %44* }
%40 = type { %41 }
%41 = type { %42 }
%42 = type { %43 }
%43 = type { %44* }
%44 = type { %6, %45, i32, %47, %37* }
%45 = type { %46 }
%46 = type { %43, %44* }
%47 = type { %48 }
%48 = type { %49, %53* }
%49 = type { %50 }
%50 = type { %51 }
%51 = type { %52 }
%52 = type { %53* }
%53 = type { %11, %54, %44*, %56 }
%54 = type { %55 }
%55 = type { %52, %53* }
%56 = type { %57 }
%57 = type { %58 }
%58 = type { %59* }
%59 = type { i8, i8, i16, i32 }
%60 = type { %61 }
%61 = type { %62, %66* }
%62 = type { %63 }
%63 = type { %64 }
%64 = type { %65 }
%65 = type { %66* }
%66 = type { %6, %67, %37* }
%67 = type { %65, %66* }
%68 = type { %69* }
%69 = type opaque
%70 = type { %71 }
%71 = type { %72, %77* }
%72 = type { %73 }
%73 = type { %74 }
%74 = type { %75 }
%75 = type { %76, %77* }
%76 = type { %77* }
%77 = type { %78, %75 }
%78 = type { %9 }
%79 = type { %80 }
%80 = type { %81, %86* }
%81 = type { %82 }
%82 = type { %83 }
%83 = type { %84 }
%84 = type { %85, %86* }
%85 = type { %86* }
%86 = type { %78, %84 }
%87 = type { %88 }
%88 = type { %89, %92* }
%89 = type { %90 }
%90 = type { %91, %92* }
%91 = type { %92* }
%92 = type { %90, %12, %18*, i8* }
%93 = type opaque
%94 = type <{ %95, %97, [3 x i8] }>
%95 = type { %96**, i32, i32, i32, i32 }
%96 = type { i32 }
%97 = type { i8 }
%98 = type { %99 }
%99 = type { %100 }
%100 = type { %101* }
%101 = type opaque
%102 = type { i8, i32, i8, [3 x i8], %103, %111, %12, %118, i8* }
%103 = type { %104, %110 }
%104 = type { %105 }
%105 = type { %106 }
%106 = type <{ %107, %108 }>
%107 = type { i8*, i8*, i8* }
%108 = type { %109 }
%109 = type { [1 x i8] }
%110 = type { [7 x %108] }
%111 = type { %112, %117 }
%112 = type { %113 }
%113 = type { %114 }
%114 = type { %107, %115 }
%115 = type { %116 }
%116 = type { [8 x i8] }
%117 = type { [15 x %115] }
%118 = type { %119, %124 }
%119 = type { %120 }
%120 = type { %121 }
%121 = type { %107, %122 }
%122 = type { %123 }
%123 = type { [16 x i8] }
%124 = type { [7 x %122] }

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start(i64, i8* nocapture) #0

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end(i64, i8* nocapture) #0

; Function Attrs: optsize
define %0* @LLVMGetReturnType(%0*) #1 {
  %2 = alloca %1*, align 4
  %3 = bitcast %1** %2 to i8*
  call void @llvm.lifetime.start(i64 4, i8* %3)
  %4 = bitcast %1** %2 to %0**
  store %0* %0, %0** %4, align 4
  %5 = call zeroext i1 @_ZN4llvm13isa_impl_wrapINS_12FunctionTypeEKPNS_4TypeEPKS2_E4doitERS4_(%1** nonnull dereferenceable(4) %2) #3
  %6 = bitcast %0* %0 to %4*
  br i1 %5, label %8, label %7

; <label>:7:                                      ; preds = %1
  tail call void @__assert_fail.5.0.1.2_i32_237.3() #4
  br label %8

; <label>:8:                                      ; preds = %7, %1
  call void @llvm.lifetime.end(i64 4, i8* %3)
  %9 = getelementptr inbounds %4, %4* %6, i32 0, i32 0, i32 3
  %10 = bitcast %1*** %9 to %0***
  %11 = load %0**, %0*** %10, align 4
  %12 = load %0*, %0** %11, align 4
  ret %0* %12
}

; Function Attrs: optsize
declare void @__assert_fail.5.0.1.2_i32_237.3() #2

; Function Attrs: optsize
define i32 @LLVMHasUnnamedAddr(%5*) #1 {
  %2 = alloca %6*, align 4
  %3 = bitcast %6** %2 to i8*
  call void @llvm.lifetime.start(i64 4, i8* %3)
  %4 = bitcast %6** %2 to %5**
  store %5* %0, %5** %4, align 4
  %5 = call zeroext i1 @_ZN4llvm13isa_impl_wrapINS_11GlobalValueEKPNS_5ValueEPKS2_E4doitERS4_(%6** nonnull dereferenceable(4) %2) #3
  %6 = bitcast %5* %0 to %9*
  br i1 %5, label %8, label %7

; <label>:7:                                      ; preds = %1
  tail call void @__assert_fail.5.0.1.2_i32_237.3() #4
  br label %8

; <label>:8:                                      ; preds = %7, %1
  call void @llvm.lifetime.end(i64 4, i8* %3)
  %9 = getelementptr inbounds %9, %9* %6, i32 0, i32 2
  %10 = load i32, i32* %9, align 4
  %11 = lshr i32 %10, 6
  %12 = and i32 %11, 1
  ret i32 %12
}

; Function Attrs: optsize
declare zeroext i1 @_ZN4llvm13isa_impl_wrapINS_12FunctionTypeEKPNS_4TypeEPKS2_E4doitERS4_(%1** nocapture readonly dereferenceable(4)) #1 align 2

; Function Attrs: optsize
declare zeroext i1 @_ZN4llvm13isa_impl_wrapINS_11GlobalValueEKPNS_5ValueEPKS2_E4doitERS4_(%6** nocapture readonly dereferenceable(4)) #1 align 2

attributes #0 = { argmemonly nounwind }
attributes #1 = { optsize }
attributes #2 = { optsize }
attributes #3 = { optsize }
attributes #4 = { noinline optsize }
