; This test checks if debug loc is propagated to load/store created by GVN/Instcombine.
; RUN: opt < %s -gvn -S | FileCheck %s --check-prefixes=ALL,GVN
; RUN: opt < %s -gvn -instcombine -S | FileCheck %s --check-prefixes=ALL,INSTCOMBINE

; struct node {
;  int  *v;
; struct desc *descs;
; };

; struct desc {
;  struct node *node;
; };

; extern int bar(void *v, void* n);

; int test(struct desc *desc)
; {
;  void *v, *n;
;  v = !desc ? ((void *)0) : desc->node->v;
;  n = &desc->node->descs[0];
;  return bar(v, n);
; }

; dbg !30 is:
; v = !desc ? ((void *)0) : desc->node->v;
;                                 ^

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64--linux-gnu"

%struct.desc = type { %struct.node* }
%struct.node = type { i32*, %struct.desc* }

define i32 @test(%struct.desc* readonly %desc) local_unnamed_addr #0 !dbg !7 {
entry:
  tail call void @llvm.dbg.value(metadata %struct.desc* %desc, i64 0, metadata !22, metadata !26), !dbg !27
  %tobool = icmp eq %struct.desc* %desc, null, !dbg !28
  br i1 %tobool, label %cond.end, label %cond.false, !dbg !29
; ALL: br i1 %tobool, label %entry.cond.end_crit_edge, label %cond.false, !dbg !29
; ALL: entry.cond.end_crit_edge:
; GVN: %.pre = load %struct.node*, %struct.node** null, align 8, !dbg !30
; INSTCOMBINE:store %struct.node* undef, %struct.node** null, align 536870912, !dbg !30

cond.false:                                       ; preds = %entry
  %0 = bitcast %struct.desc* %desc to i8***, !dbg !30
  %1 = load i8**, i8*** %0, align 8, !dbg !30, !tbaa !31
  %2 = load i8*, i8** %1, align 8, !dbg !36, !tbaa !37
  br label %cond.end, !dbg !29

cond.end:                                         ; preds = %entry, %cond.false
  %3 = phi i8* [ %2, %cond.false ], [ null, %entry ], !dbg !29
  tail call void @llvm.dbg.value(metadata i8* %3, i64 0, metadata !23, metadata !26), !dbg !39
  %node2 = getelementptr inbounds %struct.desc, %struct.desc* %desc, i64 0, i32 0, !dbg !40
  %4 = load %struct.node*, %struct.node** %node2, align 8, !dbg !40, !tbaa !31
  %descs = getelementptr inbounds %struct.node, %struct.node* %4, i64 0, i32 1, !dbg !41
  %5 = bitcast %struct.desc** %descs to i8**, !dbg !41
  %6 = load i8*, i8** %5, align 8, !dbg !41, !tbaa !42
  tail call void @llvm.dbg.value(metadata i8* %6, i64 0, metadata !25, metadata !26), !dbg !43
  %call = tail call i32 @bar(i8* %3, i8* %6) #3, !dbg !44
  ret i32 %call, !dbg !45
}

declare i32 @bar(i8*, i8*) local_unnamed_addr #1

;  Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, i64, metadata, metadata) #2

attributes #0 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="generic" "target-features"="+neon" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="generic" "target-features"="+neon" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone speculatable }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!llvm.ident = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 5.0.0 ", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "test.c", directory: "/prj/llvm-arm/home/weimingz/llvm_tests/undefined_opt")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"clang version 5.0.0 "}
!7 = distinct !DISubprogram(name: "test", scope: !1, file: !1, line: 12, type: !8, isLocal: false, isDefinition: true, scopeLine: 13, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !21)
!8 = !DISubroutineType(types: !9)
!9 = !{!10, !11}
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !12, size: 64)
!12 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "desc", file: !1, line: 6, size: 64, elements: !13)
!13 = !{!14}
!14 = !DIDerivedType(tag: DW_TAG_member, name: "node", scope: !12, file: !1, line: 7, baseType: !15, size: 64)
!15 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !16, size: 64)
!16 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "node", file: !1, line: 1, size: 128, elements: !17)
!17 = !{!18, !20}
!18 = !DIDerivedType(tag: DW_TAG_member, name: "v", scope: !16, file: !1, line: 2, baseType: !19, size: 64)
!19 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !10, size: 64)
!20 = !DIDerivedType(tag: DW_TAG_member, name: "descs", scope: !16, file: !1, line: 3, baseType: !11, size: 64, offset: 64)
!21 = !{!22, !23, !25}
!22 = !DILocalVariable(name: "desc", arg: 1, scope: !7, file: !1, line: 12, type: !11)
!23 = !DILocalVariable(name: "v", scope: !7, file: !1, line: 14, type: !24)
!24 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!25 = !DILocalVariable(name: "n", scope: !7, file: !1, line: 14, type: !24)
!26 = !DIExpression()
!27 = !DILocation(line: 12, column: 23, scope: !7)
!28 = !DILocation(line: 15, column: 7, scope: !7)
!29 = !DILocation(line: 15, column: 6, scope: !7)
!30 = !DILocation(line: 15, column: 34, scope: !7)
!31 = !{!32, !33, i64 0}
!32 = !{!"desc", !33, i64 0}
!33 = !{!"any pointer", !34, i64 0}
!34 = !{!"omnipotent char", !35, i64 0}
!35 = !{!"Simple C/C++ TBAA"}
!36 = !DILocation(line: 15, column: 40, scope: !7)
!37 = !{!38, !33, i64 0}
!38 = !{!"node", !33, i64 0, !33, i64 8}
!39 = !DILocation(line: 14, column: 8, scope: !7)
!40 = !DILocation(line: 16, column: 13, scope: !7)
!41 = !DILocation(line: 16, column: 19, scope: !7)
!42 = !{!38, !33, i64 8}
!43 = !DILocation(line: 14, column: 12, scope: !7)
!44 = !DILocation(line: 17, column: 9, scope: !7)
!45 = !DILocation(line: 17, column: 2, scope: !7)

