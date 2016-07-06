; This used to fail the verifier with the following error:
; "dbg attachment points at wrong subprogram for function"
; RUN: opt -S -mergesimilarfunc < %s | FileCheck %s
; REQUIRES: asserts
; CHECK-LABEL: @bar__merged(
target datalayout = "e-m:e-p:32:32:32-a:0-n16:32-i64:64:64-i32:32:32-i16:16:16-i1:8:8-f32:32:32-f64:64:64-v32:32:32-v64:64:64-v512:512:512-v1024:1024:1024-v2048:2048:2048"

%struct.str_type = type { i8*, i32, i32 }

; Function Attrs: nounwind optsize
define i32 @bar(i8* %b) #0 !dbg !14 {
entry:
  %retval = alloca i32, align 4
  %b.addr = alloca i8*, align 4
  %res = alloca i32, align 4
  %ee = alloca %struct.str_type*, align 4
  %cleanup.dest.slot = alloca i32
  store i8* %b, i8** %b.addr, align 4, !tbaa !31
  call void @llvm.dbg.declare(metadata i8** %b.addr, metadata !18, metadata !35), !dbg !36
  %0 = bitcast i32* %res to i8*, !dbg !37
  call void @llvm.lifetime.start(i64 4, i8* %0) #4, !dbg !37
  call void @llvm.dbg.declare(metadata i32* %res, metadata !19, metadata !35), !dbg !38
  %1 = bitcast %struct.str_type** %ee to i8*, !dbg !39
  call void @llvm.lifetime.start(i64 4, i8* %1) #4, !dbg !39
  call void @llvm.dbg.declare(metadata %struct.str_type** %ee, metadata !20, metadata !35), !dbg !40
  %2 = load i8*, i8** %b.addr, align 4, !dbg !41, !tbaa !31
  %3 = bitcast i8* %2 to %struct.str_type*, !dbg !42
  store %struct.str_type* %3, %struct.str_type** %ee, align 4, !dbg !40, !tbaa !31
  %4 = load %struct.str_type*, %struct.str_type** %ee, align 4, !dbg !43, !tbaa !31
  %set = getelementptr inbounds %struct.str_type, %struct.str_type* %4, i32 0, i32 1, !dbg !45
  %5 = load i32, i32* %set, align 4, !dbg !45, !tbaa !46
  %tobool = icmp ne i32 %5, 0, !dbg !43
  br i1 %tobool, label %if.end4, label %if.then, !dbg !49

if.then:                                          ; preds = %entry
  %6 = load %struct.str_type*, %struct.str_type** %ee, align 4, !dbg !50, !tbaa !31
  %set1 = getelementptr inbounds %struct.str_type, %struct.str_type* %6, i32 0, i32 1, !dbg !52
  store i32 1, i32* %set1, align 4, !dbg !53, !tbaa !46
  %7 = load %struct.str_type*, %struct.str_type** %ee, align 4, !dbg !54, !tbaa !31
  %x = getelementptr inbounds %struct.str_type, %struct.str_type* %7, i32 0, i32 0, !dbg !55
  %8 = load i8*, i8** %x, align 4, !dbg !55, !tbaa !56
  %call = call i32 @foo(i8* %8) #5, !dbg !57
  store i32 %call, i32* %res, align 4, !dbg !58, !tbaa !59
  %9 = load i32, i32* %res, align 4, !dbg !60, !tbaa !59
  %tobool2 = icmp ne i32 %9, 0, !dbg !60
  br i1 %tobool2, label %if.then3, label %if.end, !dbg !62

if.then3:                                         ; preds = %if.then
  store i32 1, i32* %retval, align 4, !dbg !63
  store i32 1, i32* %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !63

if.end:                                           ; preds = %if.then
  br label %if.end4, !dbg !64

if.end4:                                          ; preds = %if.end, %entry
  store i32 0, i32* %retval, align 4, !dbg !65
  store i32 1, i32* %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !65

cleanup:                                          ; preds = %if.end4, %if.then3
  %10 = bitcast %struct.str_type** %ee to i8*, !dbg !66
  call void @llvm.lifetime.end(i64 4, i8* %10) #4, !dbg !66
  %11 = bitcast i32* %res to i8*, !dbg !66
  call void @llvm.lifetime.end(i64 4, i8* %11) #4, !dbg !66
  %12 = load i32, i32* %retval, align 4, !dbg !66
  ret i32 %12, !dbg !66
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start(i64, i8* nocapture) #2

; Function Attrs: optsize
declare i32 @foo(i8*) #3

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end(i64, i8* nocapture) #2

; Function Attrs: nounwind optsize
define i32 @bar1(i8* %b) #0 !dbg !21 {
entry:
  %retval = alloca i32, align 4
  %b.addr = alloca i8*, align 4
  %res = alloca i32, align 4
  %ee = alloca %struct.str_type*, align 4
  %cleanup.dest.slot = alloca i32
  store i8* %b, i8** %b.addr, align 4, !tbaa !31
  call void @llvm.dbg.declare(metadata i8** %b.addr, metadata !23, metadata !35), !dbg !67
  %0 = bitcast i32* %res to i8*, !dbg !68
  call void @llvm.lifetime.start(i64 4, i8* %0) #4, !dbg !68
  call void @llvm.dbg.declare(metadata i32* %res, metadata !24, metadata !35), !dbg !69
  %1 = bitcast %struct.str_type** %ee to i8*, !dbg !70
  call void @llvm.lifetime.start(i64 4, i8* %1) #4, !dbg !70
  call void @llvm.dbg.declare(metadata %struct.str_type** %ee, metadata !25, metadata !35), !dbg !71
  %2 = load i8*, i8** %b.addr, align 4, !dbg !72, !tbaa !31
  %3 = bitcast i8* %2 to %struct.str_type*, !dbg !73
  store %struct.str_type* %3, %struct.str_type** %ee, align 4, !dbg !71, !tbaa !31
  %4 = load %struct.str_type*, %struct.str_type** %ee, align 4, !dbg !74, !tbaa !31
  %get = getelementptr inbounds %struct.str_type, %struct.str_type* %4, i32 0, i32 2, !dbg !76
  %5 = load i32, i32* %get, align 4, !dbg !76, !tbaa !77
  %tobool = icmp ne i32 %5, 0, !dbg !74
  br i1 %tobool, label %if.end4, label %if.then, !dbg !78

if.then:                                          ; preds = %entry
  %6 = load %struct.str_type*, %struct.str_type** %ee, align 4, !dbg !79, !tbaa !31
  %get1 = getelementptr inbounds %struct.str_type, %struct.str_type* %6, i32 0, i32 2, !dbg !81
  store i32 1, i32* %get1, align 4, !dbg !82, !tbaa !77
  %7 = load %struct.str_type*, %struct.str_type** %ee, align 4, !dbg !83, !tbaa !31
  %x = getelementptr inbounds %struct.str_type, %struct.str_type* %7, i32 0, i32 0, !dbg !84
  %8 = load i8*, i8** %x, align 4, !dbg !84, !tbaa !56
  %call = call i32 @foo(i8* %8) #5, !dbg !85
  store i32 %call, i32* %res, align 4, !dbg !86, !tbaa !59
  %9 = load i32, i32* %res, align 4, !dbg !87, !tbaa !59
  %tobool2 = icmp ne i32 %9, 0, !dbg !87
  br i1 %tobool2, label %if.then3, label %if.end, !dbg !89

if.then3:                                         ; preds = %if.then
  store i32 1, i32* %retval, align 4, !dbg !90
  store i32 1, i32* %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !90

if.end:                                           ; preds = %if.then
  br label %if.end4, !dbg !91

if.end4:                                          ; preds = %if.end, %entry
  store i32 0, i32* %retval, align 4, !dbg !92
  store i32 1, i32* %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !92

cleanup:                                          ; preds = %if.end4, %if.then3
  %10 = bitcast %struct.str_type** %ee to i8*, !dbg !93
  call void @llvm.lifetime.end(i64 4, i8* %10) #4, !dbg !93
  %11 = bitcast i32* %res to i8*, !dbg !93
  call void @llvm.lifetime.end(i64 4, i8* %11) #4, !dbg !93
  %12 = load i32, i32* %retval, align 4, !dbg !93
  ret i32 %12, !dbg !93
}

attributes #0 = { nounwind optsize }
attributes #1 = { nounwind readnone }
attributes #2 = { argmemonly nounwind }
attributes #3 = { optsize }
attributes #4 = { nounwind }
attributes #5 = { optsize }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!26, !27}
!llvm.ident = !{!28}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "Clang $LLVM_VERSION_MAJOR.$LLVM_VERSION_MINOR (based on LLVM 3.9.0)", isOptimized: true, runtimeVersion: 0, emissionKind: 1, enums: !2, retainedTypes: !3)
!1 = !DIFile(filename: "test.i", directory: "/local/mnt/")
!2 = !{}
!3 = !{!4}
!4 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !5, size: 32, align: 32)
!5 = !DIDerivedType(tag: DW_TAG_typedef, name: "str_type", file: !1, line: 8, baseType: !6)
!6 = !DICompositeType(tag: DW_TAG_structure_type, name: "str_type", file: !1, line: 3, size: 96, align: 32, elements: !7)
!7 = !{!8, !10, !12}
!8 = !DIDerivedType(tag: DW_TAG_member, name: "x", scope: !6, file: !1, line: 5, baseType: !9, size: 32, align: 32)
!9 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 32, align: 32)
!10 = !DIDerivedType(tag: DW_TAG_member, name: "set", scope: !6, file: !1, line: 6, baseType: !11, size: 32, align: 32, offset: 32)
!11 = !DIBasicType(name: "int", size: 32, align: 32, encoding: DW_ATE_signed)
!12 = !DIDerivedType(tag: DW_TAG_member, name: "get", scope: !6, file: !1, line: 7, baseType: !11, size: 32, align: 32, offset: 64)
!14 = distinct !DISubprogram(name: "bar", scope: !1, file: !1, line: 10, type: !15, isLocal: false, isDefinition: true, scopeLine: 10, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !17)
!15 = !DISubroutineType(types: !16)
!16 = !{!11, !9}
!17 = !{!18, !19, !20}
!18 = !DILocalVariable(name: "b", arg: 1, scope: !14, file: !1, line: 10, type: !9)
!19 = !DILocalVariable(name: "res", scope: !14, file: !1, line: 11, type: !11)
!20 = !DILocalVariable(name: "ee", scope: !14, file: !1, line: 12, type: !4)
!21 = distinct !DISubprogram(name: "bar1", scope: !1, file: !1, line: 24, type: !15, isLocal: false, isDefinition: true, scopeLine: 24, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !22)
!22 = !{!23, !24, !25}
!23 = !DILocalVariable(name: "b", arg: 1, scope: !21, file: !1, line: 24, type: !9)
!24 = !DILocalVariable(name: "res", scope: !21, file: !1, line: 25, type: !11)
!25 = !DILocalVariable(name: "ee", scope: !21, file: !1, line: 26, type: !4)
!26 = !{i32 2, !"Dwarf Version", i32 4}
!27 = !{i32 2, !"Debug Info Version", i32 3}
!28 = !{!"Clang $LLVM_VERSION_MAJOR.$LLVM_VERSION_MINOR (based on LLVM 3.9.0)"}
!31 = !{!32, !32, i64 0}
!32 = !{!"any pointer", !33, i64 0}
!33 = !{!"omnipotent char", !34, i64 0}
!34 = !{!"Simple C/C++ TBAA"}
!35 = !DIExpression()
!36 = !DILocation(line: 10, column: 23, scope: !14)
!37 = !DILocation(line: 11, column: 3, scope: !14)
!38 = !DILocation(line: 11, column: 7, scope: !14)
!39 = !DILocation(line: 12, column: 3, scope: !14)
!40 = !DILocation(line: 12, column: 13, scope: !14)
!41 = !DILocation(line: 12, column: 31, scope: !14)
!42 = !DILocation(line: 12, column: 19, scope: !14)
!43 = !DILocation(line: 14, column: 8, scope: !44)
!44 = distinct !DILexicalBlock(scope: !14, file: !1, line: 14, column: 7)
!45 = !DILocation(line: 14, column: 12, scope: !44)
!46 = !{!47, !48, i64 4}
!47 = !{!"str_type", !32, i64 0, !48, i64 4, !48, i64 8}
!48 = !{!"int", !33, i64 0}
!49 = !DILocation(line: 14, column: 7, scope: !14)
!50 = !DILocation(line: 15, column: 5, scope: !51)
!51 = distinct !DILexicalBlock(scope: !44, file: !1, line: 14, column: 18)
!52 = !DILocation(line: 15, column: 9, scope: !51)
!53 = !DILocation(line: 15, column: 13, scope: !51)
!54 = !DILocation(line: 17, column: 16, scope: !51)
!55 = !DILocation(line: 17, column: 20, scope: !51)
!56 = !{!47, !32, i64 0}
!57 = !DILocation(line: 17, column: 11, scope: !51)
!58 = !DILocation(line: 17, column: 9, scope: !51)
!59 = !{!48, !48, i64 0}
!60 = !DILocation(line: 18, column: 9, scope: !61)
!61 = distinct !DILexicalBlock(scope: !51, file: !1, line: 18, column: 9)
!62 = !DILocation(line: 18, column: 9, scope: !51)
!63 = !DILocation(line: 19, column: 7, scope: !61)
!64 = !DILocation(line: 20, column: 3, scope: !51)
!65 = !DILocation(line: 21, column: 3, scope: !14)
!66 = !DILocation(line: 22, column: 1, scope: !14)
!67 = !DILocation(line: 24, column: 24, scope: !21)
!68 = !DILocation(line: 25, column: 3, scope: !21)
!69 = !DILocation(line: 25, column: 7, scope: !21)
!70 = !DILocation(line: 26, column: 3, scope: !21)
!71 = !DILocation(line: 26, column: 13, scope: !21)
!72 = !DILocation(line: 26, column: 31, scope: !21)
!73 = !DILocation(line: 26, column: 19, scope: !21)
!74 = !DILocation(line: 28, column: 8, scope: !75)
!75 = distinct !DILexicalBlock(scope: !21, file: !1, line: 28, column: 7)
!76 = !DILocation(line: 28, column: 12, scope: !75)
!77 = !{!47, !48, i64 8}
!78 = !DILocation(line: 28, column: 7, scope: !21)
!79 = !DILocation(line: 29, column: 5, scope: !80)
!80 = distinct !DILexicalBlock(scope: !75, file: !1, line: 28, column: 17)
!81 = !DILocation(line: 29, column: 9, scope: !80)
!82 = !DILocation(line: 29, column: 13, scope: !80)
!83 = !DILocation(line: 31, column: 16, scope: !80)
!84 = !DILocation(line: 31, column: 20, scope: !80)
!85 = !DILocation(line: 31, column: 11, scope: !80)
!86 = !DILocation(line: 31, column: 9, scope: !80)
!87 = !DILocation(line: 32, column: 9, scope: !88)
!88 = distinct !DILexicalBlock(scope: !80, file: !1, line: 32, column: 9)
!89 = !DILocation(line: 32, column: 9, scope: !80)
!90 = !DILocation(line: 33, column: 7, scope: !88)
!91 = !DILocation(line: 34, column: 3, scope: !80)
!92 = !DILocation(line: 35, column: 3, scope: !21)
!93 = !DILocation(line: 36, column: 1, scope: !21)
