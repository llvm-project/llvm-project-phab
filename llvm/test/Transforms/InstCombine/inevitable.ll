; RUN: opt < %s -instcombine -S | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%"core::slice::Iter<f32>" = type { float*, [0 x i8], float*, [0 x i8], %"core::marker::PhantomData<&f32>", [0 x i8] }
%"core::marker::PhantomData<&f32>" = type {}

; Check that `assume` propagates backwards through operations
; that should be `isSafeToSpeculativelyExecute`.

; Check that assume is propagated backwards through all
; operations that are `isSafeToSpeculativelyExecute`
; (it should reach the load and mark it as `align 32`).
define i32 @assume_inevitable(i32* %a, i32* %b, i8* %c) {
; CHECK-LABEL: @assume_inevitable
; CHECK-DAG: load i32, i32* %a, align 32
; CHECK-DAG: call void @llvm.assume
; CHECK: ret i32
entry:
  %dummy = alloca i8, align 4
  %m = alloca i64
  %0 = load i32, i32* %a, align 4

  ; START perform a bunch of inevitable operations
  %loadres = load i32, i32* %b
  %loadres2 = call i32 @llvm.annotation.i32(i32 %loadres, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str1, i32 0, i32 0), i32 2)
  store i32 %loadres2, i32* %a

  %dummy_eq = icmp ugt i32 %loadres, 42
  tail call void @llvm.assume(i1 %dummy_eq)

  call void @llvm.lifetime.start.p0i8(i64 1, i8* %dummy)
  %i = call {}* @llvm.invariant.start.p0i8(i64 1, i8* %dummy)
  call void @llvm.invariant.end.p0i8({}* %i, i64 1, i8* %dummy)
  call void @llvm.lifetime.end.p0i8(i64 1, i8* %dummy)

  %m_i8 = bitcast i64* %m to i8*
  %m_a = call i8* @llvm.ptr.annotation.p0i8(i8* %m_i8, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str1, i32 0, i32 0), i32 2)
  %m_x = bitcast i8* %m_a to i64*
  %objsz = call i64 @llvm.objectsize.i64.p0i8(i8* %c, i1 false)
  store i64 %objsz, i64* %m_x
  ; END perform a bunch of inevitable operations

  ; AND here's the assume:
  %ptrint = ptrtoint i32* %a to i64
  %maskedptr = and i64 %ptrint, 31
  %maskcond = icmp eq i64 %maskedptr, 0
  tail call void @llvm.assume(i1 %maskcond)

  ret i32 %0
}

; Function Attrs: nounwind uwtable
; Separate test for `dbg`, copied from a Rust test because debuginfo
; is annoying to set up.
;
; ALSO there should be a similar test for `@llvm.dbg.declare`, but I'm too
; lazy to set up the needed debuginfo.
define zeroext i1 @dbg_inevitable(%"core::slice::Iter<f32>"* noalias nocapture readonly dereferenceable(16)) unnamed_addr !dbg !4 {
; CHECK-LABEL: @dbg_inevitable
; CHECK: %arg0_0 = load float*, float** %arg0_0p, align 8, !nonnull
start:
  %x = alloca %"core::slice::Iter<f32>"

  %arg0_0p = getelementptr inbounds %"core::slice::Iter<f32>", %"core::slice::Iter<f32>"* %0, i64 0, i32 0
  %arg0_0 = load float*, float** %arg0_0p, align 8
  %arg0_1p = getelementptr inbounds %"core::slice::Iter<f32>", %"core::slice::Iter<f32>"* %0, i64 0, i32 2
  %arg0_1 = load float*, float** %arg0_1p, align 8

  tail call void @llvm.dbg.declare(metadata %"core::slice::Iter<f32>"* %x, metadata !25, metadata !48), !dbg !49
  tail call void @llvm.dbg.value(metadata %"core::slice::Iter<f32>"* undef, i64 0, metadata !43, metadata !48), !dbg !51

  %assume = icmp ne float* %arg0_0, null
  call void @llvm.assume(i1 %assume)

  call void @f(%"core::slice::Iter<f32>"* %x)

  %1 = icmp eq float* %arg0_0, %arg0_1, !dbg !52
  ret i1 %1, !dbg !53
}

declare void @f(%"core::slice::Iter<f32>"*)
declare void @llvm.dbg.declare(metadata, metadata, metadata)
declare void @llvm.dbg.value(metadata, i64, metadata, metadata)

@.str = private unnamed_addr constant [4 x i8] c"sth\00", section "llvm.metadata"
@.str1 = private unnamed_addr constant [4 x i8] c"t.c\00", section "llvm.metadata"

declare i64 @llvm.objectsize.i64.p0i8(i8*, i1)
declare i32 @llvm.annotation.i32(i32, i8*, i8*, i32)
declare i8* @llvm.ptr.annotation.p0i8(i8*, i8*, i8*, i32)

declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture)
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture)

declare {}* @llvm.invariant.start.p0i8(i64, i8* nocapture)
declare void @llvm.invariant.end.p0i8({}*, i64, i8* nocapture)
declare void @llvm.assume(i1)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3}

!0 = distinct !DICompileUnit(language: DW_LANG_Rust, file: !1, producer: "clang LLVM (rustc version 1.21.0-dev)", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "./src/test/codegen/issue-37945.rs", directory: "/home/ariel/Rust/rust")
!2 = !{}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = distinct !DISubprogram(name: "is_empty_1", linkageName: "_ZN11issue_3794510is_empty_1E", scope: !5, file: !1, line: 26, type: !7, isLocal: false, isDefinition: true, scopeLine: 26, flags: DIFlagPrototyped, isOptimized: true, unit: !0, templateParams: !2, variables: !21)
!5 = !DINamespace(name: "issue_37945", scope: null)
!6 = !DIFile(filename: "<unknown>", directory: "")
!7 = !DISubroutineType(types: !8)
!8 = !{!9, !10}
!9 = !DIBasicType(name: "bool", size: 8, encoding: DW_ATE_boolean)
!10 = !DICompositeType(tag: DW_TAG_structure_type, name: "Iter<f32>", scope: !11, file: !6, size: 128, align: 64, elements: !13, identifier: "db26f7246dbf4d3fea477baae07e629cad86dfac")
!11 = !DINamespace(name: "slice", scope: !12)
!12 = !DINamespace(name: "core", scope: null)
!13 = !{!14, !17, !18}
!14 = !DIDerivedType(tag: DW_TAG_member, name: "ptr", scope: !10, file: !6, baseType: !15, size: 64, align: 64)
!15 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "*const f32", baseType: !16, size: 64, align: 64)
!16 = !DIBasicType(name: "f32", size: 32, encoding: DW_ATE_float)
!17 = !DIDerivedType(tag: DW_TAG_member, name: "end", scope: !10, file: !6, baseType: !15, size: 64, align: 64, offset: 64)
!18 = !DIDerivedType(tag: DW_TAG_member, name: "_marker", scope: !10, file: !6, baseType: !19, align: 8, offset: 128)
!19 = !DICompositeType(tag: DW_TAG_structure_type, name: "PhantomData<&f32>", scope: !20, file: !6, align: 8, elements: !2, identifier: "dbd5f37565c63321edcb3709745f241325946caa")
!20 = !DINamespace(name: "marker", scope: !12)
!21 = !{!22, !23}
!22 = !DILocalVariable(name: "xs", arg: 1, scope: !4, file: !1, line: 1, type: !10)
!23 = !DILocalVariable(name: "xs", scope: !24, file: !1, line: 26, type: !10, align: 8)
!24 = distinct !DILexicalBlock(scope: !4, file: !1, line: 26, column: 41)
!25 = !DILocalVariable(name: "self", arg: 1, scope: !26, file: !47, line: 1, type: !39)
!26 = distinct !DISubprogram(name: "next<f32>", linkageName: "_ZN4core5slice8{{impl}}9next<f32>E", scope: !28, file: !27, line: 1134, type: !29, isLocal: false, isDefinition: true, scopeLine: 1134, flags: DIFlagPrototyped, isOptimized: true, unit: !0, templateParams: !40, variables: !42)
!27 = !DIFile(filename: "/home/ariel/Rust/rust/src/libcore/slice/mod.rs", directory: "")
!28 = !DINamespace(name: "{{impl}}", scope: !11)
!29 = !DISubroutineType(types: !30)
!30 = !{!31, !39}
!31 = !DICompositeType(tag: DW_TAG_union_type, name: "Option<&f32>", scope: !32, file: !6, size: 64, align: 64, elements: !33, identifier: "8d312f71e6edb57139512d3e70760956bbe178e7")
!32 = !DINamespace(name: "option", scope: !12)
!33 = !{!34}
!34 = !DIDerivedType(tag: DW_TAG_member, name: "RUST$ENCODED$ENUM$0$None", scope: !31, file: !6, baseType: !35, size: 64, align: 64)
!35 = !DICompositeType(tag: DW_TAG_structure_type, name: "Some", scope: !32, file: !6, size: 64, align: 64, elements: !36, identifier: "8d312f71e6edb57139512d3e70760956bbe178e7::Some")
!36 = !{!37}
!37 = !DIDerivedType(tag: DW_TAG_member, name: "__0", scope: !35, file: !6, baseType: !38, size: 64, align: 64)
!38 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "&f32", baseType: !16, size: 64, align: 64)
!39 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "&mut core::slice::Iter<f32>", baseType: !10, size: 64, align: 64)
!40 = !{!41}
!41 = !DITemplateTypeParameter(name: "T", type: !16)
!42 = !{!25, !43, !45}
!43 = !DILocalVariable(name: "self", scope: !44, file: !27, line: 1134, type: !39, align: 8)
!44 = distinct !DILexicalBlock(scope: !26, file: !27, line: 1134, column: 48)
!45 = !DILocalVariable(name: "ptr", scope: !46, file: !27, line: 247, type: !15, align: 8)
!46 = distinct !DILexicalBlock(scope: !44, file: !27, line: 247, column: 23)
!47 = !DIFile(filename: "./src/test/codegen/issue-37945.rs", directory: "")
!48 = !DIExpression()
!49 = !DILocation(line: 1, scope: !26, inlinedAt: !50)
!50 = distinct !DILocation(line: 28, scope: !24)
!51 = !DILocation(line: 1134, scope: !44, inlinedAt: !50)
!52 = !DILocation(line: 1141, scope: !44, inlinedAt: !50)
!53 = !DILocation(line: 29, scope: !24)
