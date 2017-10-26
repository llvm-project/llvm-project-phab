; RUN: opt < %s -print-callgraph -disable-output 2>&1 | FileCheck %s -check-prefix=CG
; RUN: opt < %s -print-callgraph-sccs -disable-output 2>&1 | FileCheck %s -check-prefix=SCC

; CG:       Call graph node <<null function>><<{{.+}}>> #uses=0
; CG-NEXT:    CS<0x0> calls function 'main'
; CG-NEXT:    CS<0x0> calls function 'add'
; CG-NEXT:    CS<0x0> calls function 'sub'
;
; CG:       Call graph node for function: 'add'<<{{.+}}>> #uses=2
;
; CG:       Call graph node for function: 'main'<<{{.+}}>> #uses=1
; CG-NEXT:    CS<{{.+}}> calls <<null function>><<[[CALLEES:.+]]>>
;
; CG:       Call graph node for function: 'sub'<<{{.+}}>> #uses=2
;
; CG:       Call graph node <<null function>><<[[CALLEES]]>> #uses=1
; CG-NEXT:    CS<0x0> calls function 'add'
; CG-NEXT:    CS<0x0> calls function 'sub'
;
; SCC:      SCCs for the program in PostOrder
; SCC-DAG:  SCC #1 : add
; SCC-DAG:  SCC #2 : sub
; SCC:      SCC #3 : external node
; SCC-NEXT: SCC #4 : main
; SCC-NEXT: SCC #5 : external node

define i64 @main(i64 %x, i64 %y, i64 (i64, i64)* %binop) {
  %tmp0 = call i64 %binop(i64 %x, i64 %y), !callees !0
  ret i64 %tmp0
}

define i64 @add(i64 %x, i64 %y) {
  %tmp0 = add i64 %x, %y
  ret i64 %tmp0
}

define i64 @sub(i64 %x, i64 %y) {
  %tmp0 = sub i64 %x, %y
  ret i64 %tmp0
}

!0 = !{i64 (i64, i64)* @add, i64 (i64, i64)* @sub}
