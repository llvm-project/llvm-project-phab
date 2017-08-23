; RUN: llc -stop-after=livedebugvalues %s -o - | FileCheck %s

; Live debug values should be able to join two memory locations together in its
; dataflow.

; Original C source before modification to use dbg.value+DW_OP_deref, which we
; wish to generate in the frontend eventually:
; int getint(void);
; void escape(int *);
; int join_memlocs(int cond) {
;   int x = getint();
;   if (cond) {
;     x = 42;     // Dead store, can delete
;     dostuff(x); // Can propagate 42
;     x = getint();
;   }
;   escape(&x); // Escape variable to force it in memory
;   return x;
; }

; ModuleID = 't.c'
source_filename = "t.c"
target datalayout = "e-m:w-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc19.0.24215"

define i32 @join_memlocs(i32 %cond) !dbg !8 {
entry:
  %x = alloca i32, align 4
  tail call void @llvm.dbg.value(metadata i32* %x, metadata !14, metadata !DIExpression(DW_OP_deref)), !dbg !19
  %call = tail call i32 @getint(), !dbg !18
  store i32 %call, i32* %x, align 4, !dbg !19, !tbaa !20
  %tobool = icmp eq i32 %cond, 0, !dbg !24
  br i1 %tobool, label %if.end, label %if.then, !dbg !26

if.then:                                          ; preds = %entry
  tail call void @llvm.dbg.value(metadata i32 42, metadata !14, metadata !DIExpression()), !dbg !36
  tail call void @useint(i32 42), !dbg !27
  %call1 = tail call i32 @getint(), !dbg !29
  store i32 %call1, i32* %x, align 4, !dbg !30, !tbaa !20
  tail call void @llvm.dbg.value(metadata i32* %x, metadata !14, metadata !DIExpression(DW_OP_deref)), !dbg !30
  br label %if.end, !dbg !31

if.end:                                           ; preds = %entry, %if.then
  call void @escape(i32* nonnull %x), !dbg !32
  %rv = load i32, i32* %x, align 4, !dbg !33, !tbaa !20
  ret i32 %rv, !dbg !35
}

; CHECK-LABEL: name: join_memlocs
; CHECK: body: |
; CHECK:   bb.0.entry:
; CHECK:     DBG_VALUE %rsp, 0, !{{[0-9]+}}, !DIExpression(DW_OP_plus_uconst, {{[0-9]+}}, DW_OP_deref), debug-location !{{[0-9]+}}
; CHECK:   bb.1.if.then:
; CHECK:     DBG_VALUE 42, 0, !{{[0-9]+}}, !DIExpression(), debug-location !{{[0-9]+}}
; CHECK:   bb.2.if.end:
; CHECK:     DBG_VALUE debug-use %rsp, 0, !{{[0-9]+}}, !DIExpression(DW_OP_plus_uconst, {{[0-9]+}}, DW_OP_deref), debug-location !{{[0-9]+}}


; Make sure we account for the offset so we don't join unrelated memory locations.
define i32 @join_fail(i32 %cond) !dbg !40 {
entry:
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  tail call void @llvm.dbg.value(metadata i32* %x, metadata !42, metadata !DIExpression(DW_OP_deref)), !dbg !41
  %call = tail call i32 @getint()
  store i32 %call, i32* %x, align 4
  %tobool = icmp eq i32 %cond, 0
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  tail call void @llvm.dbg.value(metadata i32 42, metadata !42, metadata !DIExpression()), !dbg !41
  tail call void @useint(i32 42), !dbg !41
  %call1 = tail call i32 @getint(), !dbg !41
  store i32 %call1, i32* %x, align 4
  tail call void @llvm.dbg.value(metadata i32* %y, metadata !42, metadata !DIExpression(DW_OP_deref)), !dbg !41
  br label %if.end

if.end:                                           ; preds = %entry, %if.then
  call void @escape(i32* nonnull %x), !dbg !41
  %rv = load i32, i32* %x, align 4
  ret i32 %rv
}

; CHECK-LABEL: name: join_fail
; CHECK: body: |
; CHECK:   bb.0.entry:
; CHECK:     DBG_VALUE %rsp, 0, !{{[0-9]+}}, !DIExpression(DW_OP_plus_uconst, {{[0-9]+}}, DW_OP_deref), debug-location !{{[0-9]+}}
; CHECK:   bb.1.if.then:
; CHECK:     DBG_VALUE 42, 0, !{{[0-9]+}}, !DIExpression(), debug-location !{{[0-9]+}}
; CHECK:   bb.2.if.end:
; CHECK-NOT:     DBG_VALUE


declare i32 @getint()

declare void @useint(i32)

declare void @escape(i32*)

declare void @llvm.dbg.value(metadata, metadata, metadata)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5, !6}
!llvm.ident = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 6.0.0 ", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "t.c", directory: "C:\5Csrc\5Cllvm-project\5Cbuild")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 2}
!6 = !{i32 7, !"PIC Level", i32 2}
!7 = !{!"clang version 6.0.0 "}
!8 = distinct !DISubprogram(name: "join_memlocs", scope: !1, file: !1, line: 4, type: !9, isLocal: false, isDefinition: true, scopeLine: 4, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !12)
!9 = !DISubroutineType(types: !10)
!10 = !{!11, !11}
!11 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!12 = !{!13, !14}
!13 = !DILocalVariable(name: "cond", arg: 1, scope: !8, file: !1, line: 4, type: !11)
!14 = !DILocalVariable(name: "x", scope: !8, file: !1, line: 5, type: !11)
!15 = !DIExpression()
!16 = !DILocation(line: 4, column: 11, scope: !8)
!17 = !DILocation(line: 5, column: 3, scope: !8)
!18 = !DILocation(line: 5, column: 11, scope: !8)
!19 = !DILocation(line: 5, column: 7, scope: !8)
!20 = !{!21, !21, i64 0}
!21 = !{!"int", !22, i64 0}
!22 = !{!"omnipotent char", !23, i64 0}
!23 = !{!"Simple C/C++ TBAA"}
!24 = !DILocation(line: 6, column: 7, scope: !25)
!25 = distinct !DILexicalBlock(scope: !8, file: !1, line: 6, column: 7)
!26 = !DILocation(line: 6, column: 7, scope: !8)
!27 = !DILocation(line: 8, column: 5, scope: !28)
!28 = distinct !DILexicalBlock(scope: !25, file: !1, line: 6, column: 13)
!29 = !DILocation(line: 9, column: 9, scope: !28)
!30 = !DILocation(line: 9, column: 7, scope: !28)
!31 = !DILocation(line: 10, column: 3, scope: !28)
!32 = !DILocation(line: 11, column: 3, scope: !8)
!33 = !DILocation(line: 12, column: 10, scope: !8)
!34 = !DILocation(line: 13, column: 1, scope: !8)
!35 = !DILocation(line: 12, column: 3, scope: !8)
!36 = !DILocation(line: 7, column: 5, scope: !28)

!40 = distinct !DISubprogram(name: "join_memlocs", scope: !1, file: !1, line: 4, type: !9, isLocal: false, isDefinition: true, scopeLine: 4, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !12)
!41 = !DILocation(line: 42, column: 0, scope: !40)
!42 = !DILocalVariable(name: "x", scope: !40, file: !1, line: 42, type: !11)
