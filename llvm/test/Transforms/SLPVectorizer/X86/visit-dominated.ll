; RUN: opt -slp-vectorizer < %s -S | FileCheck %s
; Ensure the vectorizer can visit each block by dominated order.
; VEC_VALUE_QUALTYPE should dominate others.
; QUAL1_*(s) may be inducted by VEC_VALUE_QUALTYPE, since their pred is "entry".

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%"class.(anonymous namespace)::AtomicInfo" = type { %"class.clang::CodeGen::LValue" }
%"class.clang::QualType" = type { %"class.llvm::PointerIntPair.25" }
%"class.llvm::PointerIntPair.25" = type { i64 }
%"class.clang::CodeGen::LValue" = type { i32, i64*, %union.anon.1473, %"class.clang::QualType", %"class.clang::Qualifiers", i64, i8, [3 x i8], i64*, %"struct.clang::CodeGen::TBAAAccessInfo" }
%union.anon.1473 = type { %"class.llvm::Value"* }
%"class.llvm::Value" = type { i64*, i64*, i8, i8, i16, i32 }
%"class.clang::Qualifiers" = type { i32 }
%"struct.clang::CodeGen::TBAAAccessInfo" = type { %"class.clang::QualType", %"class.llvm::MDNode"*, i64 }
%"class.llvm::MDNode" = type { i64*, i32, i32, i64* }
%"class.clang::ExtQualsTypeCommonBase" = type { %"class.clang::Type"*, %"class.clang::QualType" }
%"class.clang::Type" = type { %"class.clang::ExtQualsTypeCommonBase", %union.anon.26 }
%union.anon.26 = type { %"class.clang::Type::AttributedTypeBitfields", [4 x i8] }
%"class.clang::Type::AttributedTypeBitfields" = type { i32 }
%"class.clang::ExtQuals" = type <{ %"class.clang::ExtQualsTypeCommonBase", %"class.llvm::FoldingSetBase::Node", %"class.clang::Qualifiers", [4 x i8] }>
%"class.llvm::FoldingSetBase::Node" = type { i8* }

define hidden fastcc void @_ZL21EmitAtomicUpdateValueRN5clang7CodeGen15CodeGenFunctionERN12_GLOBAL__N_110AtomicInfoENS0_6RValueENS0_7AddressE(%"class.(anonymous namespace)::AtomicInfo"* nocapture readonly dereferenceable(192) %Atomics) unnamed_addr {
entry:
  %agg.tmp38 = alloca %"class.clang::CodeGen::LValue", align 8
  %AtomicLVal.sroa.0.0..sroa_idx       = getelementptr inbounds %"class.(anonymous namespace)::AtomicInfo", %"class.(anonymous namespace)::AtomicInfo"* %Atomics, i64 0, i32 0, i32 0

  %AtomicLVal.sroa.5358.0..sroa_idx359 = getelementptr inbounds %"class.(anonymous namespace)::AtomicInfo", %"class.(anonymous namespace)::AtomicInfo"* %Atomics, i64 0, i32 0, i32 2, i32 0
; CHECK: [[VALUE0:%.+]] = getelementptr inbounds %"class.(anonymous namespace)::AtomicInfo", %"class.(anonymous namespace)::AtomicInfo"* %Atomics, i64 0, i32 0, i32 2, i32 0
  %AtomicLVal.sroa.8.0..sroa_idx363    = getelementptr inbounds %"class.(anonymous namespace)::AtomicInfo", %"class.(anonymous namespace)::AtomicInfo"* %Atomics, i64 0, i32 0, i32 3, i32 0, i32 0

  %AtomicLVal.sroa.0.0.copyload = load i32, i32* %AtomicLVal.sroa.0.0..sroa_idx, align 8
  %tmp = bitcast %"class.llvm::Value"** %AtomicLVal.sroa.5358.0..sroa_idx359 to i64*
; CHECK: [[TMP:%.+]] = bitcast %"class.llvm::Value"** [[VALUE0]] to i64*

  %AtomicLVal.LValue = load i64, i64* %tmp, align 8
  %AtomicLVal.QualType = load i64, i64* %AtomicLVal.sroa.8.0..sroa_idx363, align 8
; CHECK: [[VECP:%.+]] = bitcast i64* [[TMP]] to <2 x i64>*
; CHECK: [[VEC_VALUE_QUALTYPE:%.+]] = load <2 x i64>, <2 x i64>* [[VECP]], align 8

  switch i32 %AtomicLVal.sroa.0.0.copyload, label %if.else23 [
    i32 2, label %if.then
    i32 1, label %if.then11
  ]

; CHECK-LABEL: if.then11:
if.then11:                                        ; preds = %entry
; CHECK: [[QUAL1_11:%.+]] = extractelement <2 x i64> [[VEC_VALUE_QUALTYPE]], i32 1
  %and.i.i.i57 = and i64 %AtomicLVal.QualType, -16
; CHECK:       = and i64 [[QUAL1_11]], -16

  %tmp5 = inttoptr i64 %and.i.i.i57 to %"class.clang::ExtQualsTypeCommonBase"*
  %Value.i.i9.i.i.i58 = getelementptr inbounds %"class.clang::ExtQualsTypeCommonBase", %"class.clang::ExtQualsTypeCommonBase"* %tmp5, i64 0, i32 1, i32 0, i32 0
  %tmp6 = load i64, i64* %Value.i.i9.i.i.i58, align 8
  %tmp7 = and i64 %tmp6, 8
  %tobool.i.i.i.i59 = icmp eq i64 %tmp7, 0
  br i1 %tobool.i.i.i.i59, label %MakeVectorElt.exit, label %if.then.i.i.i63

; CHECK-LABEL: if.then:
if.then:                                          ; preds = %entry
; CHECK: [[QUAL1:%.+]] = extractelement <2 x i64> [[VEC_VALUE_QUALTYPE]], i32 1
  %and.i.i.i96 = and i64 %AtomicLVal.QualType, -16
; CHECK:       = and i64 [[QUAL1]], -16

  %tmp1 = inttoptr i64 %and.i.i.i96 to %"class.clang::ExtQualsTypeCommonBase"*
  %Value.i.i9.i.i.i97 = getelementptr inbounds %"class.clang::ExtQualsTypeCommonBase", %"class.clang::ExtQualsTypeCommonBase"* %tmp1, i64 0, i32 1, i32 0, i32 0
  %tmp2 = load i64, i64* %Value.i.i9.i.i.i97, align 8
  %tmp3 = and i64 %tmp2, 8
  %tobool.i.i.i.i98 = icmp eq i64 %tmp3, 0
  br i1 %tobool.i.i.i.i98, label %MakeBitfield.exit, label %if.then.i.i.i102

if.then.i.i.i102:                                 ; preds = %if.then
  %and.i.i.i.i.i.i.i99 = and i64 %tmp2, -16
  %tmp4 = inttoptr i64 %and.i.i.i.i.i.i.i99 to %"class.clang::ExtQuals"*
  %retval.sroa.0.0..sroa_idx.i.i.i.i100 = getelementptr inbounds %"class.clang::ExtQuals", %"class.clang::ExtQuals"* %tmp4, i64 0, i32 2, i32 0
  %retval.sroa.0.0.copyload.i.i.i.i101 = load i32, i32* %retval.sroa.0.0..sroa_idx.i.i.i.i100, align 8
  br label %MakeBitfield.exit

; CHECK_LABEL: MakeBitfield.exit:
MakeBitfield.exit: ; preds = %if.then.i.i.i102, %if.then
  %retval.sroa.0.0.i.i.i103 = phi i32 [ %retval.sroa.0.0.copyload.i.i.i.i101, %if.then.i.i.i102 ], [ 0, %if.then ]

  %conv.i.i.i67.i.i104 = or i64 %tmp2, %AtomicLVal.QualType
; CHECK:               = or i64 %tmp2, [[QUAL1]]

  %conv.i.i.i6.i.i105 = trunc i64 %conv.i.i.i67.i.i104 to i32
  %or.i.i.i.i106 = and i32 %conv.i.i.i6.i.i105, 7
  %or.i.i.i107 = or i32 %retval.sroa.0.0.i.i.i103, %or.i.i.i.i106
  br label %if.end35

if.then.i.i.i63:                                  ; preds = %if.then11
  %and.i.i.i.i.i.i.i60 = and i64 %tmp6, -16
  %tmp8 = inttoptr i64 %and.i.i.i.i.i.i.i60 to %"class.clang::ExtQuals"*
  %retval.sroa.0.0..sroa_idx.i.i.i.i61 = getelementptr inbounds %"class.clang::ExtQuals", %"class.clang::ExtQuals"* %tmp8, i64 0, i32 2, i32 0
  %retval.sroa.0.0.copyload.i.i.i.i62 = load i32, i32* %retval.sroa.0.0..sroa_idx.i.i.i.i61
  br label %MakeVectorElt.exit

; CHECK-LABEL:MakeVectorElt.exit:
MakeVectorElt.exit: ; preds = %if.then.i.i.i63, %if.then11
  %retval.sroa.0.0.i.i.i64 = phi i32 [ %retval.sroa.0.0.copyload.i.i.i.i62, %if.then.i.i.i63 ], [ 0, %if.then11 ]

  %conv.i.i.i67.i.i65 = or i64 %tmp6, %AtomicLVal.QualType
; CHECK:              = or i64 %tmp6, [[QUAL1_11]]

  %conv.i.i.i6.i.i66 = trunc i64 %conv.i.i.i67.i.i65 to i32
  %or.i.i.i.i67 = and i32 %conv.i.i.i6.i.i66, 7
  %or.i.i.i68 = or i32 %retval.sroa.0.0.i.i.i64, %or.i.i.i.i67
  br label %if.end35

; CHECK-LABEL: if.else23:
if.else23:                                        ; preds = %entry
; CHECK: [[QUAL1_23:%.+]] = extractelement <2 x i64> [[VEC_VALUE_QUALTYPE]], i32 1
  %and.i.i.i = and i64 %AtomicLVal.QualType, -16
; CHECK:     = and i64 [[QUAL1_23]], -16

  %tmp9 = inttoptr i64 %and.i.i.i to %"class.clang::ExtQualsTypeCommonBase"*
  %Value.i.i9.i.i.i = getelementptr inbounds %"class.clang::ExtQualsTypeCommonBase", %"class.clang::ExtQualsTypeCommonBase"* %tmp9, i64 0, i32 1, i32 0, i32 0
  %tmp10 = load i64, i64* %Value.i.i9.i.i.i, align 8
  %tmp11 = and i64 %tmp10, 8
  %tobool.i.i.i.i = icmp eq i64 %tmp11, 0
  br i1 %tobool.i.i.i.i, label %MakeExtVectorElt.exit, label %if.then.i.i.i

if.then.i.i.i:                                    ; preds = %if.else23
  br label %MakeExtVectorElt.exit

; CHECK-LABEL:MakeExtVectorElt.exit:
MakeExtVectorElt.exit: ; preds = %if.then.i.i.i, %if.else23

  %conv.i.i.i67.i.i = or i64 %tmp10, %AtomicLVal.QualType
; CHECK:            = or i64 %tmp10, [[QUAL1_23]]

  %or.i.i.i = trunc i64 %conv.i.i.i67.i.i to i32
  br label %if.end35

; CHECK-LABEL: if.end35:
if.end35:                                         ; preds = %MakeExtVectorElt.exit, %MakeVectorElt.exit, %MakeBitfield.exit
  %DesiredLVal.sroa.19.0 = phi i32 [ %or.i.i.i107, %MakeBitfield.exit ], [ %or.i.i.i68, %MakeVectorElt.exit ], [ %or.i.i.i, %MakeExtVectorElt.exit ]

  %DesiredLVal.sroa.12.0..sroa_idx287 = getelementptr inbounds %"class.clang::CodeGen::LValue", %"class.clang::CodeGen::LValue"* %agg.tmp38, i64 0, i32 2, i32 0
; CHECK: [[AGG287:%.+]]               = getelementptr inbounds %"class.clang::CodeGen::LValue", %"class.clang::CodeGen::LValue"* %agg.tmp38, i64 0, i32 2, i32 0
  %DesiredLVal.sroa.15.0..sroa_idx289 = getelementptr inbounds %"class.clang::CodeGen::LValue", %"class.clang::CodeGen::LValue"* %agg.tmp38, i64 0, i32 3, i32 0, i32 0

  %tmp14 = bitcast %"class.llvm::Value"** %DesiredLVal.sroa.12.0..sroa_idx287 to i64*
; CHECK: [[TMP14:%.+]] = bitcast %"class.llvm::Value"** [[AGG287]] to i64*

  store i64 %AtomicLVal.LValue, i64* %tmp14, align 8
  store i64 %AtomicLVal.QualType, i64* %DesiredLVal.sroa.15.0..sroa_idx289, align 8
; CHECK: [[LVALUE:%.+]] = bitcast i64* [[TMP14]] to <2 x i64>*
; CHECK: store <2 x i64> [[VEC_VALUE_QUALTYPE]], <2 x i64>* [[LVALUE]], align 8

  %DesiredLVal.sroa.19.0..sroa_idx291 = getelementptr inbounds %"class.clang::CodeGen::LValue", %"class.clang::CodeGen::LValue"* %agg.tmp38, i64 0, i32 4, i32 0
  store i32 %DesiredLVal.sroa.19.0, i32* %DesiredLVal.sroa.19.0..sroa_idx291, align 8
  ret void
}
